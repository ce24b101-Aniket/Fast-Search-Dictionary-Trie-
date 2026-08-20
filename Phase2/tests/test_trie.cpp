#include "core/trie.hpp"

#include <algorithm>
#include <gtest/gtest.h>

using fastsearch::Trie;

namespace {

std::vector<std::string> sorted(std::vector<std::string> v) {
    std::sort(v.begin(), v.end());
    return v;
}

}  // namespace

// ---------- basic insert/search ----------

TEST(TrieBasic, EmptyTrieHasNothing) {
    Trie t;
    EXPECT_EQ(t.size(), 0u);
    EXPECT_EQ(t.nodeCount(), 1u);  // just the root
    EXPECT_FALSE(t.search("anything"));
    EXPECT_FALSE(t.startsWith("a"));
}

TEST(TrieBasic, InsertAndSearchSingleWord) {
    Trie t;
    t.insert("concrete");
    EXPECT_TRUE(t.search("concrete"));
    EXPECT_EQ(t.size(), 1u);
}

TEST(TrieBasic, PrefixIsNotAWordUnlessInsertedSeparately) {
    Trie t;
    t.insert("concrete");
    EXPECT_FALSE(t.search("con"));      // "con" was never inserted as a word
    EXPECT_TRUE(t.startsWith("con"));   // but it is a valid prefix
}

TEST(TrieBasic, SearchNonexistentWord) {
    Trie t;
    t.insert("beam");
    EXPECT_FALSE(t.search("bridge"));
}

TEST(TrieBasic, DuplicateInsertionIsIdempotent) {
    Trie t;
    t.insert("steel");
    t.insert("steel");
    t.insert("steel");
    EXPECT_EQ(t.size(), 1u);
    EXPECT_TRUE(t.search("steel"));
}

TEST(TrieBasic, SharedPrefixesInsertedIndependently) {
    Trie t;
    t.insert("car");
    t.insert("care");
    t.insert("careful");
    EXPECT_TRUE(t.search("car"));
    EXPECT_TRUE(t.search("care"));
    EXPECT_TRUE(t.search("careful"));
    EXPECT_EQ(t.size(), 3u);
}

TEST(TrieBasic, EmptyWordInsertThrows) {
    Trie t;
    EXPECT_THROW(t.insert(""), std::invalid_argument);
}

TEST(TrieBasic, EmptyWordSearchReturnsFalseNotThrow) {
    Trie t;
    t.insert("cement");
    EXPECT_FALSE(t.search(""));
}

TEST(TrieBasic, InvalidCharacterThrows) {
    Trie t;
    EXPECT_THROW(t.insert("Concrete"), std::invalid_argument);  // uppercase
    EXPECT_THROW(t.insert("con crete"), std::invalid_argument); // space
    EXPECT_THROW(t.insert("con-crete"), std::invalid_argument); // punctuation
    EXPECT_THROW(t.insert("con123"), std::invalid_argument);    // digits
}

TEST(TrieBasic, VeryLongWord) {
    Trie t;
    std::string longWord(100, 'a');
    t.insert(longWord);
    EXPECT_TRUE(t.search(longWord));
}

// ---------- deletion ----------

TEST(TrieRemove, RemoveLeafWord) {
    Trie t;
    t.insert("beam");
    EXPECT_TRUE(t.remove("beam"));
    EXPECT_FALSE(t.search("beam"));
    EXPECT_EQ(t.size(), 0u);
}

TEST(TrieRemove, RemoveNonexistentWordReturnsFalse) {
    Trie t;
    t.insert("beam");
    EXPECT_FALSE(t.remove("bridge"));
    EXPECT_TRUE(t.search("beam"));  // untouched
}

TEST(TrieRemove, RemovingPrefixWordDoesNotDeleteLongerWords) {
    Trie t;
    t.insert("car");
    t.insert("care");
    t.insert("careful");

    EXPECT_TRUE(t.remove("car"));

    EXPECT_FALSE(t.search("car"));
    EXPECT_TRUE(t.search("care"));
    EXPECT_TRUE(t.search("careful"));
    EXPECT_TRUE(t.startsWith("car"));  // still a valid prefix of "care"
}

TEST(TrieRemove, RemovingLongerWordDoesNotAffectShorterPrefixWord) {
    Trie t;
    t.insert("car");
    t.insert("careful");

    EXPECT_TRUE(t.remove("careful"));

    EXPECT_TRUE(t.search("car"));
    EXPECT_FALSE(t.search("careful"));
}

TEST(TrieRemove, RemovePrunesDeadBranchNodeCount) {
    Trie t;
    t.insert("cat");
    std::size_t nodesAfterInsert = t.nodeCount();  // root + c,a,t = 4
    EXPECT_EQ(nodesAfterInsert, 4u);

    EXPECT_TRUE(t.remove("cat"));
    EXPECT_EQ(t.nodeCount(), 1u);  // fully pruned back to just root
}

TEST(TrieRemove, RemoveDoesNotPruneNodesStillUsedBySiblingWords) {
    Trie t;
    t.insert("cat");
    t.insert("car");
    // shared: c, a. "cat" branch: t. "car" branch: r.
    EXPECT_TRUE(t.remove("cat"));
    EXPECT_TRUE(t.search("car"));
    EXPECT_FALSE(t.search("cat"));
    EXPECT_TRUE(t.startsWith("ca"));
}

TEST(TrieRemove, RemoveEmptyWordReturnsFalse) {
    Trie t;
    t.insert("cement");
    EXPECT_FALSE(t.remove(""));
}

TEST(TrieRemove, RemoveThenReinsert) {
    Trie t;
    t.insert("grout");
    EXPECT_TRUE(t.remove("grout"));
    EXPECT_FALSE(t.search("grout"));
    t.insert("grout");
    EXPECT_TRUE(t.search("grout"));
    EXPECT_EQ(t.size(), 1u);
}

// ---------- autocomplete / suggestions ----------

TEST(TrieSuggestions, SuggestionsForKnownPrefix) {
    Trie t;
    for (const char* w : {"concrete", "concreting", "conduit", "column"}) {
        t.insert(w);
    }
    auto results = sorted(t.getSuggestions("con"));
    std::vector<std::string> expected = {"concrete", "concreting", "conduit"};
    EXPECT_EQ(results, sorted(expected));
}

TEST(TrieSuggestions, SuggestionsForUnknownPrefixIsEmpty) {
    Trie t;
    t.insert("beam");
    EXPECT_TRUE(t.getSuggestions("xyz").empty());
}

TEST(TrieSuggestions, SuggestionsForEmptyPrefixReturnsEveryWord) {
    Trie t;
    t.insert("beam");
    t.insert("bridge");
    t.insert("cement");
    auto results = sorted(t.getSuggestions(""));
    std::vector<std::string> expected = {"beam", "bridge", "cement"};
    EXPECT_EQ(results, sorted(expected));
}

TEST(TrieSuggestions, PrefixThatIsAlsoACompleteWordIsIncluded) {
    Trie t;
    t.insert("car");
    t.insert("care");
    t.insert("careful");
    auto results = sorted(t.getSuggestions("car"));
    std::vector<std::string> expected = {"car", "care", "careful"};
    EXPECT_EQ(results, sorted(expected));
}

TEST(TrieSuggestions, ThousandWordsSharedPrefix) {
    Trie t;
    for (int i = 0; i < 1000; ++i) {
        std::string suffix;
        int n = i;
        for (int d = 0; d < 5; ++d) {
            suffix.push_back(static_cast<char>('a' + (n % 5)));
            n /= 5;
        }
        t.insert("pre" + suffix);
    }
    EXPECT_EQ(t.size(), 1000u);
    EXPECT_EQ(t.getSuggestions("pre").size(), 1000u);
}

// ---------- size / nodeCount / clear ----------

TEST(TrieMeta, SizeTracksDistinctWordsOnly) {
    Trie t;
    t.insert("slab");
    t.insert("slab");
    t.insert("sand");
    EXPECT_EQ(t.size(), 2u);
}

TEST(TrieMeta, ClearResetsEverything) {
    Trie t;
    t.insert("granite");
    t.insert("gravel");
    t.clear();
    EXPECT_EQ(t.size(), 0u);
    EXPECT_EQ(t.nodeCount(), 1u);
    EXPECT_FALSE(t.search("granite"));
}

TEST(TrieMeta, NodeCountReflectsSharedPrefixes) {
    Trie t;
    t.insert("go");
    t.insert("gone");
    // root, g, o(=end), n, e  => 5 nodes
    EXPECT_EQ(t.nodeCount(), 5u);
}
