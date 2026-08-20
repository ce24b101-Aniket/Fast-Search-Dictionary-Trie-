#pragma once

#include <string>
#include <vector>

#include "db/database_manager.hpp"

namespace fastsearch {

struct SearchHistoryRecord {
    long long id = 0;
    std::string query;
    int resultCount = 0;
    double latencyMs = 0.0;
    std::string createdAt;
};

struct HistoryStats {
    long long totalSearches = 0;
    double averageLatencyMs = 0.0;
    long long searchesWithResults = 0;
    long long searchesWithNoResults = 0;
};

// Repository for the `search_history` table. Deliberately narrow: only
// what the analytics endpoints (spec section 21) actually need.
class SearchHistoryRepository {
public:
    explicit SearchHistoryRepository(DatabaseManager& db) : db_(db) {}

    // Records one search event.
    void record(const std::string& query, int resultCount, double latencyMs);

    // Returns the `limit` most recent searches, newest first.
    std::vector<SearchHistoryRecord> recent(int limit) const;

    // Aggregate stats over all recorded history.
    HistoryStats stats() const;

    // Top `limit` most-searched distinct queries (by occurrence count).
    std::vector<std::pair<std::string, long long>> mostSearched(int limit) const;

private:
    DatabaseManager& db_;
};

}  // namespace fastsearch
