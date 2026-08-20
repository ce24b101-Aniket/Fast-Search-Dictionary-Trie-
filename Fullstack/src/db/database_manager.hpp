#pragma once

#include <memory>
#include <string>

struct sqlite3;

namespace fastsearch {

// Owns a single SQLite connection and is responsible for creating the
// schema on first use. This is the only place in the codebase that
// knows the word "sqlite3" -- repositories take a DatabaseManager&,
// never a raw sqlite3* passed around independently, so persistence
// stays swappable in theory (even though we're not building a second
// backend, section 12 of the spec).
class DatabaseManager {
public:
    // Opens (creating if necessary) the SQLite file at `path`, or an
    // in-memory database if path == ":memory:" (used by tests).
    explicit DatabaseManager(const std::string& path);
    ~DatabaseManager();

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    // Raw handle, for repositories in this same layer to prepare
    // statements against. Not exposed outside src/db.
    sqlite3* handle() const noexcept { return db_; }

    // Runs CREATE TABLE IF NOT EXISTS / CREATE INDEX IF NOT EXISTS for
    // the words and search_history tables. Idempotent -- safe to call
    // every startup.
    void migrate();

private:
    sqlite3* db_ = nullptr;
};

}  // namespace fastsearch
