#pragma once

#include <optional>
#include <string>
#include <vector>

#include "db/database_manager.hpp"

namespace fastsearch {

struct WordRecord {
    long long id = 0;
    std::string word;
    long frequency = 0;
    std::string createdAt;
    std::string updatedAt;
};

// Repository for the `words` table. This is the ONLY place SQL for
// words lives -- the Trie doesn't know SQLite exists, and the service
// layer doesn't write SQL directly (spec section 2's layering rule).
class WordRepository {
public:
    explicit WordRepository(DatabaseManager& db) : db_(db) {}

    // Inserts a new word with frequency 0, or does nothing if it
    // already exists. Returns true if a row was actually inserted.
    bool insert(const std::string& word);

    // Returns the record for `word`, or std::nullopt if absent.
    std::optional<WordRecord> find(const std::string& word) const;

    // Sets frequency to an explicit value (also bumps updated_at).
    // Returns false if the word doesn't exist.
    bool setFrequency(const std::string& word, long frequency);

    // Increments frequency by 1. Returns false if the word doesn't
    // exist.
    bool incrementFrequency(const std::string& word);

    // Deletes the word. Returns true if a row was actually removed.
    bool remove(const std::string& word);

    // Returns every word in the table, ordered by word ascending. Used
    // once at startup to rebuild the in-memory Trie (see
    // docs/database.md for why this doesn't scale past a certain size
    // and what the next step would be).
    std::vector<WordRecord> findAll() const;

    // Total row count -- O(1) via COUNT(*), used for /api/stats.
    long long count() const;

private:
    DatabaseManager& db_;
};

}  // namespace fastsearch
