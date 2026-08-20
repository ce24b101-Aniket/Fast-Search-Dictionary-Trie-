#include "db/database_manager.hpp"

#include <sqlite3.h>

#include <stdexcept>

namespace fastsearch {

namespace {

// Schema:
//
// words: the source of truth for the dictionary. `word` is UNIQUE so
// re-inserting is a conflict the repository layer resolves explicitly
// (see WordRepository::upsert), not something SQLite silently allows
// duplicates for.
//
// search_history: one row per search request, used for the analytics
// dashboard (Phase 6/API stats endpoints). Deliberately does not store
// any user/session identifier -- see spec section 11 ("do not store
// unnecessary personal information").
constexpr const char* kSchemaSql = R"SQL(
CREATE TABLE IF NOT EXISTS words (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    word TEXT NOT NULL UNIQUE,
    frequency INTEGER NOT NULL DEFAULT 0,
    created_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ','now')),
    updated_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ','now'))
);
CREATE INDEX IF NOT EXISTS idx_words_word ON words(word);

CREATE TABLE IF NOT EXISTS search_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    query TEXT NOT NULL,
    result_count INTEGER NOT NULL,
    latency_ms REAL NOT NULL,
    created_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ','now'))
);
CREATE INDEX IF NOT EXISTS idx_history_created_at ON search_history(created_at);
)SQL";

}  // namespace

DatabaseManager::DatabaseManager(const std::string& path) {
    int rc = sqlite3_open(path.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::string msg = db_ ? sqlite3_errmsg(db_) : "unknown error";
        if (db_) sqlite3_close(db_);
        db_ = nullptr;
        throw std::runtime_error("DatabaseManager: failed to open '" + path +
                                  "': " + msg);
    }
    // WAL improves concurrent read/write behavior for a file-based DB
    // under a multi-threaded HTTP server (Phase 5/10). No-op for
    // :memory: databases.
    sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(db_, "PRAGMA foreign_keys=ON;", nullptr, nullptr, nullptr);
}

DatabaseManager::~DatabaseManager() {
    if (db_) sqlite3_close(db_);
}

void DatabaseManager::migrate() {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, kSchemaSql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::string msg = errMsg ? errMsg : "unknown error";
        sqlite3_free(errMsg);
        throw std::runtime_error("DatabaseManager::migrate failed: " + msg);
    }
}

}  // namespace fastsearch
