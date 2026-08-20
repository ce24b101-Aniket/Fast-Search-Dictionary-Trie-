#include <gtest/gtest.h>

#include "db/database_manager.hpp"
#include "db/search_history_repository.hpp"
#include "db/word_repository.hpp"

using namespace fastsearch;

namespace {

// Every test gets its own fresh in-memory database (":memory:") so
// tests can't interfere with each other and no file is left on disk.
class WordRepositoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        db = std::make_unique<DatabaseManager>(":memory:");
        db->migrate();
        repo = std::make_unique<WordRepository>(*db);
    }
    std::unique_ptr<DatabaseManager> db;
    std::unique_ptr<WordRepository> repo;
};

}  // namespace

TEST_F(WordRepositoryTest, InsertAndFind) {
    EXPECT_TRUE(repo->insert("concrete"));
    auto rec = repo->find("concrete");
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec->word, "concrete");
    EXPECT_EQ(rec->frequency, 0);
}

TEST_F(WordRepositoryTest, DuplicateInsertReturnsFalse) {
    EXPECT_TRUE(repo->insert("concrete"));
    EXPECT_FALSE(repo->insert("concrete"));  // already exists
    EXPECT_EQ(repo->count(), 1);
}

TEST_F(WordRepositoryTest, FindMissingWordReturnsNullopt) {
    EXPECT_FALSE(repo->find("nope").has_value());
}

TEST_F(WordRepositoryTest, SetFrequency) {
    repo->insert("beam");
    EXPECT_TRUE(repo->setFrequency("beam", 42));
    EXPECT_EQ(repo->find("beam")->frequency, 42);
}

TEST_F(WordRepositoryTest, SetFrequencyOnMissingWordReturnsFalse) {
    EXPECT_FALSE(repo->setFrequency("nope", 42));
}

TEST_F(WordRepositoryTest, IncrementFrequency) {
    repo->insert("beam");
    repo->incrementFrequency("beam");
    repo->incrementFrequency("beam");
    EXPECT_EQ(repo->find("beam")->frequency, 2);
}

TEST_F(WordRepositoryTest, RemoveWord) {
    repo->insert("beam");
    EXPECT_TRUE(repo->remove("beam"));
    EXPECT_FALSE(repo->find("beam").has_value());
}

TEST_F(WordRepositoryTest, RemoveMissingWordReturnsFalse) {
    EXPECT_FALSE(repo->remove("nope"));
}

TEST_F(WordRepositoryTest, FindAllReturnsEverythingSorted) {
    repo->insert("concrete");
    repo->insert("beam");
    repo->insert("bridge");
    auto all = repo->findAll();
    ASSERT_EQ(all.size(), 3u);
    EXPECT_EQ(all[0].word, "beam");
    EXPECT_EQ(all[1].word, "bridge");
    EXPECT_EQ(all[2].word, "concrete");
}

TEST_F(WordRepositoryTest, CountReflectsInsertsAndDeletes) {
    repo->insert("a");
    repo->insert("b");
    EXPECT_EQ(repo->count(), 2);
    repo->remove("a");
    EXPECT_EQ(repo->count(), 1);
}

// ---------- search history ----------

namespace {

class SearchHistoryRepositoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        db = std::make_unique<DatabaseManager>(":memory:");
        db->migrate();
        repo = std::make_unique<SearchHistoryRepository>(*db);
    }
    std::unique_ptr<DatabaseManager> db;
    std::unique_ptr<SearchHistoryRepository> repo;
};

}  // namespace

TEST_F(SearchHistoryRepositoryTest, RecordAndRetrieveRecent) {
    repo->record("con", 3, 0.5);
    repo->record("bridge", 1, 0.2);
    auto recent = repo->recent(10);
    ASSERT_EQ(recent.size(), 2u);
    EXPECT_EQ(recent[0].query, "bridge");  // most recent first
    EXPECT_EQ(recent[1].query, "con");
}

TEST_F(SearchHistoryRepositoryTest, RecentRespectsLimit) {
    for (int i = 0; i < 5; ++i) repo->record("q" + std::to_string(i), 1, 1.0);
    EXPECT_EQ(repo->recent(2).size(), 2u);
}

TEST_F(SearchHistoryRepositoryTest, StatsAggregatesCorrectly) {
    repo->record("con", 3, 1.0);
    repo->record("bridge", 0, 3.0);  // no results found
    repo->record("beam", 2, 2.0);

    auto s = repo->stats();
    EXPECT_EQ(s.totalSearches, 3);
    EXPECT_DOUBLE_EQ(s.averageLatencyMs, 2.0);
    EXPECT_EQ(s.searchesWithResults, 2);
    EXPECT_EQ(s.searchesWithNoResults, 1);
}

TEST_F(SearchHistoryRepositoryTest, StatsOnEmptyHistory) {
    auto s = repo->stats();
    EXPECT_EQ(s.totalSearches, 0);
    EXPECT_DOUBLE_EQ(s.averageLatencyMs, 0.0);
}

TEST_F(SearchHistoryRepositoryTest, MostSearchedRanksByFrequency) {
    repo->record("con", 3, 1.0);
    repo->record("con", 3, 1.0);
    repo->record("bridge", 1, 1.0);

    auto top = repo->mostSearched(5);
    ASSERT_GE(top.size(), 2u);
    EXPECT_EQ(top[0].first, "con");
    EXPECT_EQ(top[0].second, 2);
}
