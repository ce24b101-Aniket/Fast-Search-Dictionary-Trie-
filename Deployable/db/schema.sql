-- FastSearch database schema.
--
-- Design notes:
--   * `words` is the source of truth for the dictionary. The in-memory
--     Trie is a derived index rebuilt from this table on startup -- see
--     StartupLoader / SearchService::loadFromDatabase(). If the Trie and
--     the DB ever disagree, the DB wins after a restart.
--   * `frequency` lives in both places (DB and TrieNode) deliberately:
--     the Trie needs it in-memory for O(1) access during ranking; the DB
--     needs it so frequency survives a restart.
--   * `search_history` is append-only and intentionally minimal --  only
--     the query text, result count, and latency are stored. No user
--     identifiers, IP addresses, or session data, per the "don't store
--     unnecessary personal information" requirement.

PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS words (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    word        TEXT NOT NULL UNIQUE,
    frequency   INTEGER NOT NULL DEFAULT 0 CHECK (frequency >= 0),
    created_at  TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ', 'now')),
    updated_at  TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ', 'now'))
);

-- Speeds up prefix scans issued as `WHERE word LIKE 'con%'` for any
-- future admin/debug tooling that queries the DB directly instead of
-- the Trie. The Trie remains the primary search index at request time;
-- this index exists for the DB's own sake (uniqueness + admin queries),
-- not as a substitute for the Trie.
CREATE INDEX IF NOT EXISTS idx_words_word ON words(word);

CREATE TABLE IF NOT EXISTS search_history (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    query         TEXT NOT NULL,
    result_count  INTEGER NOT NULL,
    latency_ms    REAL NOT NULL,
    created_at    TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ', 'now'))
);

CREATE INDEX IF NOT EXISTS idx_search_history_created_at ON search_history(created_at);
CREATE INDEX IF NOT EXISTS idx_search_history_query ON search_history(query);
