#include "core/trie.hpp"

#include <gtest/gtest.h>

using fastsearch::Trie;

// ---------- frequency basics ----------

TEST(TrieFrequency, NewWordStartsAtZero) {
    Trie t;
    t.insert("concrete");
    EXPECT_EQ(t.getFrequency("concrete"), 0);
}

TEST(TrieFrequency, IncrementFrequency) {
    Trie t;
    t.insert("concrete");
    EXPECT_TRUE(t.incrementFrequency("concrete"));
    EXPECT_TRUE(t.incrementFrequency("concrete"));
    EXPECT_EQ(t.getFrequency("concrete"), 2);
}

TEST(TrieFrequency, IncrementNonexistentWordReturnsFalse) {
    Trie t;
    EXPECT_FALSE(t.incrementFrequency("concrete"));
}

TEST(TrieFrequency, IncrementOnPrefixThatIsNotAWordFails) {
    Trie t;
    t.insert("concrete");
    // "con" is a valid prefix but was never inserted as its own word.
    EXPECT_FALSE(t.incrementFrequency("con"));
}

TEST(TrieFrequency, SetFrequencyExplicitly) {
    Trie t;
    t.insert("concrete");
    EXPECT_TRUE(t.setFrequency("concrete", 500));
    EXPECT_EQ(t.getFrequency("concrete"), 500);
}

TEST(TrieFrequency, SetNegativeFrequencyRejected) {
    Trie t;
    t.insert("concrete");
    EXPECT_FALSE(t.setFrequency("concrete", -1));
    EXPECT_EQ(t.getFrequency("concrete"), 0);  // unchanged
}

TEST(TrieFrequency, FrequencyOfNonexistentWordIsZero) {
    Trie t;
    EXPECT_EQ(t.getFrequency("nope"), 0);
}

TEST(TrieFrequency, RemovingWordAndReinsertingResetsFrequency) {
    Trie t;
    t.insert("grout");
    t.setFrequency("grout", 42);
    t.remove("grout");
    t.insert("grout");
    // A fresh node was allocated on reinsert since the old one was
    // pruned -- frequency should not "survive" a full removal.
    EXPECT_EQ(t.getFrequency("grout"), 0);
}

// ---------- Top-K autocomplete ----------

TEST(TrieAutocomplete, RanksByFrequencyDescending) {
    Trie t;
    t.insert("concrete");
    t.insert("concreting");
    t.insert("conduit");
    t.setFrequency("concrete", 100);
    t.setFrequency("concreting", 50);
    t.setFrequency("conduit", 200);

    auto results = t.autocomplete("con", 3);
    ASSERT_EQ(results.size(), 3u);
    EXPECT_EQ(results[0].word, "conduit");
    EXPECT_EQ(results[1].word, "concrete");
    EXPECT_EQ(results[2].word, "concreting");
}

TEST(TrieAutocomplete, LimitsToK) {
    Trie t;
    t.insert("concrete");
    t.insert("concreting");
    t.insert("conduit");
    t.setFrequency("concrete", 100);
    t.setFrequency("concreting", 50);
    t.setFrequency("conduit", 200);

    auto results = t.autocomplete("con", 2);
    ASSERT_EQ(results.size(), 2u);
    EXPECT_EQ(results[0].word, "conduit");
    EXPECT_EQ(results[1].word, "concrete");
}

TEST(TrieAutocomplete, TiesBreakLexicographically) {
    Trie t;
    t.insert("beam");
    t.insert("bridge");
    t.insert("brick");
    // all frequency 0 -- must fall back to alphabetical order

    auto results = t.autocomplete("b", 3);
    ASSERT_EQ(results.size(), 3u);
    EXPECT_EQ(results[0].word, "beam");
    EXPECT_EQ(results[1].word, "brick");   // "bri**c**k" < "bri**d**ge"
    EXPECT_EQ(results[2].word, "bridge");
}

TEST(TrieAutocomplete, KZeroReturnsEmpty) {
    Trie t;
    t.insert("beam");
    EXPECT_TRUE(t.autocomplete("b", 0).empty());
}

TEST(TrieAutocomplete, KGreaterThanMatchCountReturnsAllMatches) {
    Trie t;
    t.insert("beam");
    t.insert("bridge");
    auto results = t.autocomplete("b", 100);
    EXPECT_EQ(results.size(), 2u);
}

TEST(TrieAutocomplete, UnknownPrefixReturnsEmpty) {
    Trie t;
    t.insert("beam");
    EXPECT_TRUE(t.autocomplete("xyz", 5).empty());
}

TEST(TrieAutocomplete, PrefixThatIsAlsoAWordIsIncludedAndRanked) {
    Trie t;
    t.insert("car");
    t.insert("care");
    t.insert("careful");
    t.setFrequency("car", 10);
    t.setFrequency("care", 999);
    t.setFrequency("careful", 5);

    auto results = t.autocomplete("car", 3);
    ASSERT_EQ(results.size(), 3u);
    EXPECT_EQ(results[0].word, "care");
    EXPECT_EQ(results[1].word, "car");
    EXPECT_EQ(results[2].word, "careful");
}

namespace {

// Encodes i in [0, 625) as a 4-letter base-5 (a-e) suffix, so every word
// is unique, letters-only (the Trie only accepts a-z), and the encoding
// is invertible enough to know exactly which word corresponds to the
// highest i without guessing.
std::string base5Suffix(int i) {
    std::string suffix;
    for (int d = 0; d < 4; ++d) {
        suffix.push_back(static_cast<char>('a' + (i % 5)));
        i /= 5;
    }
    return suffix;
}

}  // namespace

TEST(TrieAutocomplete, LargeDatasetTopKIsCorrect) {
    Trie t;
    // 500 words sharing a prefix, frequency == insertion index, so the
    // highest-frequency word is deterministically known and we can
    // check the heap picked it correctly out of a much larger candidate
    // set than k.
    for (int i = 0; i < 500; ++i) {
        std::string w = "pre" + base5Suffix(i);
        t.insert(w);
        t.setFrequency(w, i);
    }
    auto results = t.autocomplete("pre", 5);
    ASSERT_EQ(results.size(), 5u);
    EXPECT_EQ(results[0].word, "pre" + base5Suffix(499));
    EXPECT_EQ(results[0].frequency, 499);
    EXPECT_EQ(results[4].frequency, 495);
}
