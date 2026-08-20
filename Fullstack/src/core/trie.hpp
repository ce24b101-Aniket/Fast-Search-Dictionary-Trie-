#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/trie_node.hpp"

namespace fastsearch {

// A single ranked autocomplete result: a word plus the frequency it was
// ranked by. Kept separate from the plain vector<string> of
// getSuggestions() because callers of ranked results usually want to
// display the score too (see API response design in docs/api.md).
struct Suggestion {
    std::string word;
    long frequency;
};

// A Trie (prefix tree) restricted to lowercase a-z, storing complete
// words. This class does NOT normalize input -- callers (services,
// tests) are expected to pass already-normalized strings, typically via
// Normalizer::normalize() first. See core/normalizer.hpp.
//
// Every public method that receives a std::string still defends itself
// against invalid characters by throwing std::invalid_argument rather
// than indexing the children array out of bounds. This is a direct fix
// for a real bug in the original implementation, where `ch - 'a'` was
// used as an array index with no range check -- feeding it anything
// outside a-z was undefined behavior.
//
// Ownership: all nodes are owned via unique_ptr in a strict tree
// structure (see trie_node.hpp), so destruction is automatic and there
// is no manual destroy()/delete step.
class Trie {
public:
    Trie();

    // Non-copyable (a Trie can own thousands of nodes; copying should be
    // explicit and expensive-looking, not accidental via `=`).
    Trie(const Trie&) = delete;
    Trie& operator=(const Trie&) = delete;

    // Movable.
    Trie(Trie&&) noexcept = default;
    Trie& operator=(Trie&&) noexcept = default;

    ~Trie() = default;

    // Inserts `word`. Re-inserting an existing word is a no-op beyond
    // marking it terminal again (idempotent). Throws std::invalid_argument
    // if `word` is empty or contains a character outside a-z.
    void insert(const std::string& word);

    // Returns true if `word` was inserted as a complete word.
    // Throws std::invalid_argument for invalid characters.
    bool search(const std::string& word) const;

    // Alias for search(), kept because "does this word exist" reads more
    // naturally as `contains` in calling code.
    bool contains(const std::string& word) const;

    // Returns true if any inserted word begins with `prefix`. An empty
    // prefix matches everything in a non-empty Trie.
    bool startsWith(const std::string& prefix) const;

    // Removes `word` if it exists as a complete word. Returns true if a
    // word was actually removed, false if it wasn't present.
    //
    // Only unmarks isEndOfWord and prunes now-dead branches -- it must
    // NOT remove nodes that are still prefixes of other words. e.g.
    // after inserting "car", "care", "careful", removing "car" must
    // leave "care" and "careful" fully intact.
    bool remove(const std::string& word);

    // Returns every complete word stored in the Trie that begins with
    // `prefix`, in unspecified order, with no limit on result count.
    //
    // This is intentionally unranked and unbounded -- it exists to
    // preserve the original project's autocomplete behavior during the
    // Phase 1 rewrite. Phase 2 replaces/extends this with a bounded,
    // frequency-ranked Top-K implementation (autocomplete(prefix, k)).
    std::vector<std::string> getSuggestions(const std::string& prefix) const;

    // Returns up to `k` words beginning with `prefix`, ranked by
    // frequency descending, then lexicographically ascending for ties.
    // This is the real autocomplete algorithm: a single DFS collects
    // matching words while maintaining a min-heap of size <= k (the
    // "worst of the current best k" sits at the heap's top), so an
    // improving candidate costs one pop + one push instead of resorting
    // everything.
    //
    // For M matching words in the subtree, this is O(M log K) -- versus
    // O(M log M) for "collect everything, then std::sort" -- because
    // heap operations are bounded by K, not M. When K is close to M the
    // two approaches converge; the win shows up when K << M (e.g.
    // "give me the top 5 of 50,000 matches").
    //
    // k == 0 returns an empty vector immediately (no traversal cost).
    // There is no upper cap enforced here -- capping unreasonable client
    // requests (e.g. k == 999999999) is the API layer's job, not the
    // Trie's; see Phase 5 / docs/api.md.
    std::vector<Suggestion> autocomplete(const std::string& prefix, std::size_t k) const;

    // Returns the stored frequency for `word`, or 0 if the word doesn't
    // exist (there is no separate "not found" signal -- a nonexistent
    // word and a word with 0 recorded hits are indistinguishable by
    // design, since frequency 0 is a perfectly valid state).
    long getFrequency(const std::string& word) const;

    // Increments `word`'s frequency by 1. Returns false (no-op) if the
    // word isn't present -- this deliberately does NOT insert the word,
    // since "someone searched for a word that doesn't exist" and
    // "someone wants a new word added" are different operations that
    // callers should not conflate.
    bool incrementFrequency(const std::string& word);

    // Sets `word`'s frequency to an explicit value. Returns false if the
    // word isn't present. Rejects negative values.
    bool setFrequency(const std::string& word, long frequency);

    // Number of complete words currently stored.
    std::size_t size() const noexcept;

    // Number of TrieNode objects currently allocated (including root).
    // Useful for memory-usage analysis and benchmarking.
    std::size_t nodeCount() const noexcept;

    // Resets the Trie to empty. O(number of nodes) since the whole tree
    // is discarded and rebuilt.
    void clear();

private:
    std::unique_ptr<TrieNode> root_;
    std::size_t wordCount_ = 0;
    std::size_t nodeCount_ = 0;

    // Maps 'a'-'z' to 0-25. Throws std::invalid_argument for anything
    // else -- this is the single choke point that prevents the
    // out-of-bounds array access present in the original implementation.
    static int charToIndex(char c);

    // Walks the Trie following `prefix`. Returns nullptr if the prefix
    // path doesn't fully exist.
    const TrieNode* findNode(const std::string& prefix) const;
    TrieNode* findNodeMutable(const std::string& prefix);

    // DFS collecting every complete word (and its frequency) under
    // `node`, used by autocomplete()'s heap-selection pass.
    static void collectSuggestions(const TrieNode* node, std::string& current,
                                    std::vector<Suggestion>& results);

    // Recursive helper for remove(). Returns true if `node` (at the
    // given depth into `word`) became empty and should be unlinked by
    // its parent.
    bool removeHelper(TrieNode* node, const std::string& word, std::size_t depth,
                       bool& removed);

    // Returns true if `node` has at least one child.
    static bool hasAnyChild(const TrieNode* node) noexcept;

    // DFS collecting every complete word under `node` into `results`,
    // building the word incrementally in `current` via push_back/pop_back
    // to avoid repeated string concatenation/copying during traversal.
    static void collectWords(const TrieNode* node, std::string& current,
                              std::vector<std::string>& results);
};

}  // namespace fastsearch
