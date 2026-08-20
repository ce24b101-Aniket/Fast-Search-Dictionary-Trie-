#include "service/search_service.hpp"

#include <algorithm>
#include <chrono>
#include <mutex>

#include "core/normalizer.hpp"

namespace fastsearch {

namespace {

// RAII-style stopwatch using steady_clock (never affected by system
// clock adjustments) -- see spec section 22 on using high-resolution,
// monotonic timing for latency measurements.
class Stopwatch {
public:
    Stopwatch() : start_(std::chrono::steady_clock::now()) {}
    double elapsedMs() const {
        auto end = std::chrono::steady_clock::now();
        return std::chrono::duration<double, std::milli>(end - start_).count();
    }

private:
    std::chrono::steady_clock::time_point start_;
};

}  // namespace

std::string SearchService::normalizeOrThrow(const std::string& raw) {
    auto result = Normalizer::normalize(raw);
    if (!result.valid) {
        throw ValidationError(result.error);
    }
    return result.value;
}

SearchService::SearchService(WordRepository& wordRepo, SearchHistoryRepository& historyRepo)
    : wordRepo_(wordRepo), historyRepo_(historyRepo) {
    // Startup flow (spec section 13): rebuild the Trie entirely from
    // whatever is already in the database. This is what makes the
    // "DB succeeded, Trie update failed" failure mode self-healing --
    // the very next restart reconciles the two.
    for (const WordRecord& rec : wordRepo_.findAll()) {
        trie_.insert(rec.word);
        if (rec.frequency > 0) {
            trie_.setFrequency(rec.word, rec.frequency);
        }
    }
}

SearchResult SearchService::search(const std::string& rawWord) {
    Stopwatch sw;
    std::string word = normalizeOrThrow(rawWord);

    SearchResult result;
    result.word = word;

    {
        std::unique_lock lock(mutex_);  // may write (frequency bump) -> exclusive
        result.found = trie_.search(word);
        if (result.found) {
            trie_.incrementFrequency(word);
            result.frequency = trie_.getFrequency(word);
        }
    }

    if (result.found) {
        // Best-effort: keep DB frequency in sync with the Trie. If this
        // throws, the Trie is still correct in memory and the next
        // restart's rebuild will reconcile from the DB's last known
        // value -- documented tradeoff, not a silent bug.
        wordRepo_.incrementFrequency(word);
    }

    result.latencyMs = sw.elapsedMs();
    historyRepo_.record(word, result.found ? 1 : 0, result.latencyMs);
    return result;
}

AutocompleteResult SearchService::autocomplete(const std::string& rawPrefix, std::size_t limit) {
    Stopwatch sw;
    std::string prefix = normalizeOrThrow(rawPrefix);
    std::size_t cappedLimit = std::min(limit, kMaxAutocompleteLimit);

    AutocompleteResult result;
    result.prefix = prefix;
    {
        std::shared_lock lock(mutex_);  // read-only -> shared
        result.results = trie_.autocomplete(prefix, cappedLimit);
    }
    result.latencyMs = sw.elapsedMs();

    historyRepo_.record(prefix, static_cast<int>(result.results.size()), result.latencyMs);
    return result;
}

WordRecord SearchService::addWord(const std::string& rawWord) {
    std::string word = normalizeOrThrow(rawWord);

    // DB first (source of truth), then Trie -- consistency strategy
    // documented in the header.
    if (!wordRepo_.insert(word)) {
        throw ConflictError("word already exists: " + word);
    }

    {
        std::unique_lock lock(mutex_);
        trie_.insert(word);
    }

    return *wordRepo_.find(word);
}

WordRecord SearchService::updateWordFrequency(const std::string& rawWord, long frequency) {
    std::string word = normalizeOrThrow(rawWord);
    if (frequency < 0) {
        throw ValidationError("frequency must not be negative");
    }

    if (!wordRepo_.setFrequency(word, frequency)) {
        throw NotFoundError("word not found: " + word);
    }

    {
        std::unique_lock lock(mutex_);
        trie_.setFrequency(word, frequency);
    }

    return *wordRepo_.find(word);
}

void SearchService::deleteWord(const std::string& rawWord) {
    std::string word = normalizeOrThrow(rawWord);

    if (!wordRepo_.remove(word)) {
        throw NotFoundError("word not found: " + word);
    }

    {
        std::unique_lock lock(mutex_);
        trie_.remove(word);
    }
}

WordRecord SearchService::getWord(const std::string& rawWord) {
    std::string word = normalizeOrThrow(rawWord);
    auto rec = wordRepo_.find(word);
    if (!rec) {
        throw NotFoundError("word not found: " + word);
    }
    return *rec;
}

SearchService::Stats SearchService::getStats() {
    Stats stats;
    {
        std::shared_lock lock(mutex_);
        stats.totalWords = trie_.size();
        stats.trieNodeCount = trie_.nodeCount();
    }
    HistoryStats hs = historyRepo_.stats();
    stats.totalSearches = hs.totalSearches;
    stats.searchesWithResults = hs.searchesWithResults;
    stats.searchesWithNoResults = hs.searchesWithNoResults;
    stats.averageLatencyMs = hs.averageLatencyMs;
    return stats;
}

std::vector<SearchHistoryRecord> SearchService::getHistory(int limit) {
    return historyRepo_.recent(limit);
}

}  // namespace fastsearch
