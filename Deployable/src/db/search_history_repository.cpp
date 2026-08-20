#include "db/search_history_repository.hpp"

#include <sqlite3.h>

#include <stdexcept>

namespace fastsearch {

namespace {

class Statement {
public:
    Statement(sqlite3* db, const char* sql) {
        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt_, nullptr);
        if (rc != SQLITE_OK) {
            throw std::runtime_error(std::string("sqlite3_prepare_v2 failed: ") +
                                      sqlite3_errmsg(db));
        }
    }
    ~Statement() { sqlite3_finalize(stmt_); }
    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;
    sqlite3_stmt* get() const noexcept { return stmt_; }

private:
    sqlite3_stmt* stmt_ = nullptr;
};

}  // namespace

void SearchHistoryRepository::record(const std::string& query, int resultCount,
                                      double latencyMs) {
    Statement stmt(db_.handle(),
                    "INSERT INTO search_history (query, result_count, latency_ms) "
                    "VALUES (?, ?, ?);");
    sqlite3_bind_text(stmt.get(), 1, query.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt.get(), 2, resultCount);
    sqlite3_bind_double(stmt.get(), 3, latencyMs);
    int rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
        throw std::runtime_error(std::string("SearchHistoryRepository::record failed: ") +
                                  sqlite3_errmsg(db_.handle()));
    }
}

std::vector<SearchHistoryRecord> SearchHistoryRepository::recent(int limit) const {
    Statement stmt(db_.handle(),
                    "SELECT id, query, result_count, latency_ms, created_at "
                    "FROM search_history ORDER BY id DESC LIMIT ?;");
    sqlite3_bind_int(stmt.get(), 1, limit);
    std::vector<SearchHistoryRecord> results;
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        SearchHistoryRecord rec;
        rec.id = sqlite3_column_int64(stmt.get(), 0);
        rec.query = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 1));
        rec.resultCount = sqlite3_column_int(stmt.get(), 2);
        rec.latencyMs = sqlite3_column_double(stmt.get(), 3);
        rec.createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 4));
        results.push_back(std::move(rec));
    }
    return results;
}

HistoryStats SearchHistoryRepository::stats() const {
    HistoryStats out;
    Statement stmt(db_.handle(),
                    "SELECT COUNT(*), COALESCE(AVG(latency_ms), 0), "
                    "SUM(CASE WHEN result_count > 0 THEN 1 ELSE 0 END), "
                    "SUM(CASE WHEN result_count = 0 THEN 1 ELSE 0 END) "
                    "FROM search_history;");
    if (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        out.totalSearches = sqlite3_column_int64(stmt.get(), 0);
        out.averageLatencyMs = sqlite3_column_double(stmt.get(), 1);
        out.searchesWithResults = sqlite3_column_int64(stmt.get(), 2);
        out.searchesWithNoResults = sqlite3_column_int64(stmt.get(), 3);
    }
    return out;
}

std::vector<std::pair<std::string, long long>> SearchHistoryRepository::mostSearched(
    int limit) const {
    Statement stmt(db_.handle(),
                    "SELECT query, COUNT(*) as cnt FROM search_history "
                    "GROUP BY query ORDER BY cnt DESC LIMIT ?;");
    sqlite3_bind_int(stmt.get(), 1, limit);
    std::vector<std::pair<std::string, long long>> results;
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        std::string query = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 0));
        long long cnt = sqlite3_column_int64(stmt.get(), 1);
        results.emplace_back(std::move(query), cnt);
    }
    return results;
}

}  // namespace fastsearch
