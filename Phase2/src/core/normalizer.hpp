#pragma once

#include <string>

namespace fastsearch {

// Maximum accepted word length. This exists to protect the Trie (and any
// API built on top of it) from pathological input -- without a cap, a
// single request could insert an arbitrarily long string and allocate
// one Trie node per character.
inline constexpr std::size_t kMaxWordLength = 100;

// Result of attempting to normalize a piece of user input into the
// canonical form the Trie accepts.
struct NormalizationResult {
    std::string value;   // normalized text (only meaningful if valid == true)
    bool valid = false;
    std::string error;   // human-readable reason, only set if valid == false
};

// Converts arbitrary user input into the canonical form the Trie
// operates on: trimmed, lowercased, and restricted to a-z.
//
// LIMITATION (documented, not hidden): this only supports the 26-letter
// lowercase English alphabet. Anything containing digits, punctuation,
// whitespace in the middle, or non-ASCII/Unicode characters is rejected
// as invalid rather than silently mangled. Unicode support would require
// a case-folding library (not just tolower) and a Trie child
// representation that isn't a fixed 26-slot array -- see
// docs/trie-design-comparison.md.
class Normalizer {
public:
    static NormalizationResult normalize(const std::string& input);
};

}  // namespace fastsearch
