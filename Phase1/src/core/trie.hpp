#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/trie_node.hpp"

namespace fastsearch {

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
