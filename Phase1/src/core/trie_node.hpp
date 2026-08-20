#pragma once

#include <array>
#include <memory>

namespace fastsearch {

// Number of children slots per node. Fixed at 26 because the current
// alphabet is restricted to lowercase English letters (a-z). See
// docs/trie-design-comparison.md for why this was chosen over a
// std::unordered_map<char, ...> child representation, and what would
// need to change to support a larger alphabet (digits, Unicode, etc).
inline constexpr int kAlphabetSize = 26;

// A single node in the Trie.
//
// Ownership model: each node owns its children via unique_ptr, so the
// whole tree is destroyed automatically and recursively when the root
// unique_ptr goes out of scope -- no manual delete/destroy() needed,
// unlike the original raw-pointer implementation.
struct TrieNode {
    std::array<std::unique_ptr<TrieNode>, kAlphabetSize> children;
    bool isEndOfWord = false;

    TrieNode() = default;

    // Nodes are owned exclusively via unique_ptr in a tree; copying or
    // moving a node independently of its parent doesn't make sense, so
    // both are disabled to avoid accidental double-ownership bugs.
    TrieNode(const TrieNode&) = delete;
    TrieNode& operator=(const TrieNode&) = delete;
    TrieNode(TrieNode&&) = delete;
    TrieNode& operator=(TrieNode&&) = delete;
};

}  // namespace fastsearch
