#include "db/word_repository.hpp"

#include <sqlite3.h>

#include <stdexcept>

namespace fastsearch {

namespace {

// Small RAII wrapper so every function below gets automatic
// sqlite3_finalize() even on early return/throw, without repeating
// try/finally-style cleanup by hand at every call site.
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

WordRecord rowToRecord(sqlite3_stmt* stmt) {
    WordRecord rec;
    rec.id = sqlite3_column_int64(stmt, 0);
    rec.word = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    rec.frequency = sqlite3_column_int64(stmt, 2);
    rec.createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    rec.updatedAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
    return rec;
}

}  // namespace

bool WordRepository::insert(const std::string& word) {
    Statement stmt(db_.handle(),
                    "INSERT OR IGNORE INTO words (word) VALUES (?);");
    sqlite3_bind_text(stmt.get(), 1, word.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
        throw std::runtime_error(std::string("WordRepository::insert failed: ") +
                                  sqlite3_errmsg(db_.handle()));
    }
    return sqlite3_changes(db_.handle()) > 0;
}

std::optional<WordRecord> WordRepository::find(const std::string& word) const {
    Statement stmt(db_.handle(),
                    "SELECT id, word, frequency, created_at, updated_at "
                    "FROM words WHERE word = ?;");
    sqlite3_bind_text(stmt.get(), 1, word.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt.get());
    if (rc == SQLITE_ROW) {
        return rowToRecord(stmt.get());
    }
    return std::nullopt;
}

bool WordRepository::setFrequency(const std::string& word, long frequency) {
    Statement stmt(db_.handle(),
                    "UPDATE words SET frequency = ?, "
                    "updated_at = strftime('%Y-%m-%dT%H:%M:%fZ','now') "
                    "WHERE word = ?;");
    sqlite3_bind_int64(stmt.get(), 1, frequency);
    sqlite3_bind_text(stmt.get(), 2, word.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
        throw std::runtime_error(std::string("WordRepository::setFrequency failed: ") +
                                  sqlite3_errmsg(db_.handle()));
    }
    return sqlite3_changes(db_.handle()) > 0;
}

bool WordRepository::incrementFrequency(const std::string& word) {
    Statement stmt(db_.handle(),
                    "UPDATE words SET frequency = frequency + 1, "
                    "updated_at = strftime('%Y-%m-%dT%H:%M:%fZ','now') "
                    "WHERE word = ?;");
    sqlite3_bind_text(stmt.get(), 1, word.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
        throw std::runtime_error(
            std::string("WordRepository::incrementFrequency failed: ") +
            sqlite3_errmsg(db_.handle()));
    }
    return sqlite3_changes(db_.handle()) > 0;
}

bool WordRepository::remove(const std::string& word) {
    Statement stmt(db_.handle(), "DELETE FROM words WHERE word = ?;");
    sqlite3_bind_text(stmt.get(), 1, word.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt.get());
    if (rc != SQLITE_DONE) {
        throw std::runtime_error(std::string("WordRepository::remove failed: ") +
                                  sqlite3_errmsg(db_.handle()));
    }
    return sqlite3_changes(db_.handle()) > 0;
}

std::vector<WordRecord> WordRepository::findAll() const {
    Statement stmt(db_.handle(),
                    "SELECT id, word, frequency, created_at, updated_at "
                    "FROM words ORDER BY word ASC;");
    std::vector<WordRecord> results;
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        results.push_back(rowToRecord(stmt.get()));
    }
    return results;
}

long long WordRepository::count() const {
    Statement stmt(db_.handle(), "SELECT COUNT(*) FROM words;");
    sqlite3_step(stmt.get());
    return sqlite3_column_int64(stmt.get(), 0);
}

}  // namespace fastsearch
