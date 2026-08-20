#pragma once

#include <shared_mutex>
#include <string>
#include <vector>

#include "core/trie.hpp"
#include "db/search_history_repository.hpp"
#include "db/word_repository.hpp"

namespace fastsearch {

struct SearchResult {
    std::string word;
    bool found = false;
    long frequency = 0;
    double latencyMs = 0.0;
};

struct AutocompleteResult {
    std::string prefix;
    std::vector<Suggestion> results;
    double latencyMs = 0.0;
};

// Error thrown for validation failures (bad word, bad limit, etc). The
// API layer (src/api) catches this and turns it into a 400/422 JSON
// response -- the service layer itself knows nothing about HTTP.
class ValidationError : public std::runtime_error {
public:
    explicit ValidationError(const std::string& msg) : std::runtime_error(msg) {}
};

// Thrown when an operation targets a word that doesn't exist (e.g.
// DELETE on a missing word) -- the API layer maps this to 404.
class NotFoundError : public std::runtime_error {
public:
    explicit NotFoundError(const std::string& msg) : std::runtime_error(msg) {}
};

// Thrown when an operation conflicts with existing state (e.g. POSTing
// a word that already exists) -- the API layer maps this to 409.
class ConflictError : public std::runtime_error {
public:
    explicit ConflictError(const std::string& msg) : std::runtime_error(msg) {}
};

// The layer that knows about BOTH the Trie and the database, and
// coordinates them. Nothing above this layer (the API) touches the Trie
// or the repositories directly -- see spec section 2's layering rule.
//
// Consistency strategy (spec section 14): the database is the source of
// truth. Every write goes to the database first; the Trie is updated
// only after the database write succeeds. If the process restarts, the
// Trie is always rebuilt from the database from scratch (see
// SearchService's constructor), so a Trie update that somehow failed
// after a successful DB write is self-healing on the next restart. This
// is a deliberate, documented tradeoff against building distributed
// transactions for a single-process, single-database system where none
// are warranted (spec section 59).
//
// Concurrency (spec section 29): the Trie is read-mostly under a typical
// HTTP workload (many autocomplete/search requests, few writes), so it's
// guarded by a std::shared_mutex -- concurrent readers proceed in
// parallel; a writer (insert/remove/frequency update) gets exclusive
// access. The SQLite connection itself is guarded by the same lock for
// simplicity; SQLite's own locking would also serialize writers, but
// keeping one mutex for both keeps the DB-then-Trie write ordering
// trivially atomic from every other thread's point of view.
class SearchService {
public:
    // Loads every word from `wordRepo` and builds the in-memory Trie
    // from scratch. This is the startup flow from spec section 13:
    // Database -> Load dictionary -> Build Trie -> ready to serve.
    SearchService(WordRepository& wordRepo, SearchHistoryRepository& historyRepo);

    // Exact search. Increments the word's frequency (both DB and Trie)
    // on a hit, and always records the query in search history.
    SearchResult search(const std::string& rawWord);

    // Ranked autocomplete, capped at kMaxAutocompleteLimit regardless of
    // what the caller asks for (spec section 7: "protect the API
    // against unreasonable limits"). Records the query in search
    // history.
    AutocompleteResult autocomplete(const std::string& rawPrefix, std::size_t limit);

    // Adds a new word. Throws ConflictError if it already exists,
    // ValidationError if the input is invalid.
    WordRecord addWord(const std::string& rawWord);

    // Updates a word's frequency to an explicit value. Throws
    // NotFoundError if it doesn't exist.
    WordRecord updateWordFrequency(const std::string& rawWord, long frequency);

    // Deletes a word. Throws NotFoundError if it doesn't exist.
    void deleteWord(const std::string& rawWord);

    // Looks up a word's current record without affecting frequency or
    // search history (used by GET /api/words/:word).
    WordRecord getWord(const std::string& rawWord);

    // Aggregate stats combining Trie size/nodeCount, word count, and
    // search history stats -- see docs/api.md for the exact JSON shape.
    struct Stats {
        std::size_t totalWords = 0;
        std::size_t trieNodeCount = 0;
        long long totalSearches = 0;
        long long searchesWithResults = 0;
        long long searchesWithNoResults = 0;
        double averageLatencyMs = 0.0;
    };
    Stats getStats();

    std::vector<SearchHistoryRecord> getHistory(int limit);

    // Maximum k accepted by autocomplete, regardless of what a client
    // requests. Chosen to be generous for a UI dropdown while still
    // bounding worst-case work per request.
    static constexpr std::size_t kMaxAutocompleteLimit = 50;

private:
    WordRepository& wordRepo_;
    SearchHistoryRepository& historyRepo_;
    Trie trie_;
    mutable std::shared_mutex mutex_;

    // Throws ValidationError if invalid; returns the normalized word.
    static std::string normalizeOrThrow(const std::string& raw);
};

}  // namespace fastsearch
