#include "service/search_service.hpp"

#include <gtest/gtest.h>

#include "db/database_manager.hpp"

using namespace fastsearch;

namespace {

class SearchServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        db = std::make_unique<DatabaseManager>(":memory:");
        db->migrate();
        wordRepo = std::make_unique<WordRepository>(*db);
        historyRepo = std::make_unique<SearchHistoryRepository>(*db);
        service = std::make_unique<SearchService>(*wordRepo, *historyRepo);
    }
    std::unique_ptr<DatabaseManager> db;
    std::unique_ptr<WordRepository> wordRepo;
    std::unique_ptr<SearchHistoryRepository> historyRepo;
    std::unique_ptr<SearchService> service;
};

}  // namespace

TEST_F(SearchServiceTest, AddWordThenSearchFindsIt) {
    service->addWord("Concrete");  // uppercase, exercises normalization
    auto result = service->search("concrete");
    EXPECT_TRUE(result.found);
    EXPECT_EQ(result.word, "concrete");
}

TEST_F(SearchServiceTest, SearchMissesUpdateHistoryButNotFrequency) {
    auto result = service->search("nonexistent");
    EXPECT_FALSE(result.found);
    EXPECT_EQ(result.frequency, 0);
}

TEST_F(SearchServiceTest, SearchIncrementsFrequencyInBothTrieAndDb) {
    service->addWord("beam");
    service->search("beam");
    service->search("beam");
    auto rec = service->getWord("beam");
    EXPECT_EQ(rec.frequency, 2);  // persisted in DB
}

TEST_F(SearchServiceTest, AddDuplicateWordThrowsConflict) {
    service->addWord("beam");
    EXPECT_THROW(service->addWord("beam"), ConflictError);
    EXPECT_THROW(service->addWord("BEAM"), ConflictError);  // same after normalization
}

TEST_F(SearchServiceTest, AddInvalidWordThrowsValidationError) {
    EXPECT_THROW(service->addWord(""), ValidationError);
    EXPECT_THROW(service->addWord("con crete"), ValidationError);
    EXPECT_THROW(service->addWord("con123"), ValidationError);
}

TEST_F(SearchServiceTest, DeleteNonexistentWordThrowsNotFound) {
    EXPECT_THROW(service->deleteWord("nope"), NotFoundError);
}

TEST_F(SearchServiceTest, DeleteWordRemovesFromBothTrieAndDb) {
    service->addWord("grout");
    service->deleteWord("grout");
    EXPECT_FALSE(service->search("grout").found);
    EXPECT_THROW(service->getWord("grout"), NotFoundError);
}

TEST_F(SearchServiceTest, UpdateFrequencyOnMissingWordThrowsNotFound) {
    EXPECT_THROW(service->updateWordFrequency("nope", 10), NotFoundError);
}

TEST_F(SearchServiceTest, UpdateFrequencyNegativeThrowsValidation) {
    service->addWord("slab");
    EXPECT_THROW(service->updateWordFrequency("slab", -5), ValidationError);
}

TEST_F(SearchServiceTest, AutocompleteRanksByFrequency) {
    service->addWord("concrete");
    service->addWord("concreting");
    service->addWord("conduit");
    service->updateWordFrequency("conduit", 200);
    service->updateWordFrequency("concrete", 100);

    auto result = service->autocomplete("con", 5);
    ASSERT_GE(result.results.size(), 2u);
    EXPECT_EQ(result.results[0].word, "conduit");
}

TEST_F(SearchServiceTest, AutocompleteLimitIsCappedRegardlessOfRequest) {
    for (int i = 0; i < 60; ++i) {
        char c1 = static_cast<char>('a' + (i % 26));
        char c2 = static_cast<char>('a' + ((i / 26) % 26));
        service->addWord(std::string("word") + c1 + c2);
    }
    // Ask for far more than kMaxAutocompleteLimit -- must be capped, not
    // crash or return an unbounded amount of work.
    auto result = service->autocomplete("word", 999999);
    EXPECT_LE(result.results.size(), SearchService::kMaxAutocompleteLimit);
}

TEST_F(SearchServiceTest, StartupRebuildsTrieFromExistingDatabaseRows) {
    // Simulate data already in the DB before a SearchService (and its
    // Trie) exists -- e.g. a prior run of the server.
    wordRepo->insert("cement");
    wordRepo->setFrequency("cement", 77);
    wordRepo->insert("bitumen");

    SearchService fresh(*wordRepo, *historyRepo);
    auto result = fresh.search("cement");
    EXPECT_TRUE(result.found);
    EXPECT_GE(result.frequency, 77);  // search() itself increments by 1 more
    EXPECT_TRUE(fresh.search("bitumen").found);
}

TEST_F(SearchServiceTest, StatsReflectWordsAndHistory) {
    service->addWord("beam");
    service->addWord("bridge");
    service->search("beam");
    service->search("nonexistent");

    auto stats = service->getStats();
    EXPECT_EQ(stats.totalWords, 2u);
    EXPECT_EQ(stats.totalSearches, 2);
    EXPECT_EQ(stats.searchesWithResults, 1);
    EXPECT_EQ(stats.searchesWithNoResults, 1);
}

TEST_F(SearchServiceTest, HistoryReturnsRecentSearches) {
    service->addWord("beam");
    service->search("beam");
    service->autocomplete("be", 5);

    auto history = service->getHistory(10);
    EXPECT_EQ(history.size(), 2u);
}
