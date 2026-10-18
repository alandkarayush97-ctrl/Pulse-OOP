#pragma once
// RAII wrapper around SQLite. Encapsulates all C API handling.
#include <sqlite3.h>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#include "nlohmann/json.hpp"

using Json = nlohmann::json;

class Database {
  sqlite3* db_ = nullptr;

  void bindAll(sqlite3_stmt* st, const std::vector<Json>& p) const {
    for (size_t i = 0; i < p.size(); ++i) {
      int idx = static_cast<int>(i) + 1;
      const Json& v = p[i];
      if (v.is_null()) sqlite3_bind_null(st, idx);
      else if (v.is_boolean()) sqlite3_bind_int(st, idx, v.get<bool>() ? 1 : 0);
      else if (v.is_number_integer()) sqlite3_bind_int64(st, idx, v.get<long long>());
      else if (v.is_number()) sqlite3_bind_double(st, idx, v.get<double>());
      else if (v.is_string()) { std::string s = v.get<std::string>(); sqlite3_bind_text(st, idx, s.c_str(), static_cast<int>(s.size()), SQLITE_TRANSIENT); }
      else { std::string s = v.dump(); sqlite3_bind_text(st, idx, s.c_str(), static_cast<int>(s.size()), SQLITE_TRANSIENT); }
    }
  }

 public:
  explicit Database(const std::string& path) {
    if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) throw std::runtime_error("Cannot open database");
    rawExec("PRAGMA journal_mode = WAL; PRAGMA synchronous = NORMAL; PRAGMA foreign_keys = ON;");
  }
  ~Database() { if (db_) sqlite3_close(db_); }
  Database(const Database&) = delete;
  Database& operator=(const Database&) = delete;

  void rawExec(const std::string& sql) {
    char* err = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
      std::string m = err ? err : "sqlite error"; sqlite3_free(err); throw std::runtime_error(m);
    }
  }

  // SELECT -> rows as JSON objects (column name -> value)
  std::vector<Json> query(const std::string& sql, const std::vector<Json>& params = {}) {
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &st, nullptr) != SQLITE_OK) throw std::runtime_error(sqlite3_errmsg(db_));
    bindAll(st, params);
    std::vector<Json> rows;
    int rc;
    while ((rc = sqlite3_step(st)) == SQLITE_ROW) {
      Json row = Json::object();
      for (int c = 0; c < sqlite3_column_count(st); ++c) {
        const char* name = sqlite3_column_name(st, c);
        switch (sqlite3_column_type(st, c)) {
          case SQLITE_INTEGER: row[name] = static_cast<long long>(sqlite3_column_int64(st, c)); break;
          case SQLITE_FLOAT: row[name] = sqlite3_column_double(st, c); break;
          case SQLITE_NULL: row[name] = nullptr; break;
          default: row[name] = std::string(reinterpret_cast<const char*>(sqlite3_column_text(st, c)),
                                           static_cast<size_t>(sqlite3_column_bytes(st, c)));
        }
      }
      rows.push_back(std::move(row));
    }
    std::string err = sqlite3_errmsg(db_);
    sqlite3_finalize(st);
    if (rc != SQLITE_DONE) throw std::runtime_error(err);
    return rows;
  }

  std::optional<Json> one(const std::string& sql, const std::vector<Json>& params = {}) {
    auto rows = query(sql, params);
    if (rows.empty()) return std::nullopt;
    return rows.front();
  }

  // INSERT/UPDATE/DELETE -> last inserted row id
  long long exec(const std::string& sql, const std::vector<Json>& params = {}) {
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &st, nullptr) != SQLITE_OK) throw std::runtime_error(sqlite3_errmsg(db_));
    bindAll(st, params);
    int rc = sqlite3_step(st);
    std::string err = sqlite3_errmsg(db_);
    sqlite3_finalize(st);
    if (rc != SQLITE_DONE && rc != SQLITE_ROW) throw std::runtime_error(err);
    return static_cast<long long>(sqlite3_last_insert_rowid(db_));
  }

  long long scalar(const std::string& sql, const std::vector<Json>& params = {}) {
    auto r = one(sql, params);
    if (!r) return 0;
    const Json& v = r->begin().value();
    return v.is_number() ? v.get<long long>() : 0;
  }

  void initSchema() {
    rawExec(R"SQL(
CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL, handle TEXT UNIQUE NOT NULL,
  email TEXT UNIQUE NOT NULL, password TEXT NOT NULL, bio TEXT DEFAULT '', avatar_url TEXT DEFAULT NULL, created_at TEXT DEFAULT CURRENT_TIMESTAMP);
CREATE TABLE IF NOT EXISTS tweets (id INTEGER PRIMARY KEY AUTOINCREMENT, author_id INTEGER NOT NULL REFERENCES users(id),
  content TEXT NOT NULL, vibe TEXT DEFAULT 'Wholesome', energy TEXT DEFAULT 'Just sharing', publish_at TEXT DEFAULT NULL,
  image_url TEXT DEFAULT NULL, reply_to INTEGER REFERENCES tweets(id), created_at TEXT DEFAULT CURRENT_TIMESTAMP);
CREATE TABLE IF NOT EXISTS likes (user_id INTEGER NOT NULL REFERENCES users(id), tweet_id INTEGER NOT NULL REFERENCES tweets(id), PRIMARY KEY (user_id, tweet_id));
CREATE TABLE IF NOT EXISTS retweets (user_id INTEGER NOT NULL REFERENCES users(id), tweet_id INTEGER NOT NULL REFERENCES tweets(id), PRIMARY KEY (user_id, tweet_id));
CREATE TABLE IF NOT EXISTS reactions (user_id INTEGER NOT NULL REFERENCES users(id), tweet_id INTEGER NOT NULL REFERENCES tweets(id), emoji TEXT NOT NULL, PRIMARY KEY (user_id, tweet_id));
CREATE TABLE IF NOT EXISTS follows (follower_id INTEGER NOT NULL REFERENCES users(id), following_id INTEGER NOT NULL REFERENCES users(id), PRIMARY KEY (follower_id, following_id));
CREATE TABLE IF NOT EXISTS notifications (id INTEGER PRIMARY KEY AUTOINCREMENT, user_id INTEGER NOT NULL REFERENCES users(id), type TEXT NOT NULL,
  actor_id INTEGER NOT NULL REFERENCES users(id), tweet_id INTEGER REFERENCES tweets(id), read INTEGER DEFAULT 0, created_at TEXT DEFAULT CURRENT_TIMESTAMP);
CREATE TABLE IF NOT EXISTS tweet_coauthors (tweet_id INTEGER NOT NULL REFERENCES tweets(id), user_id INTEGER NOT NULL REFERENCES users(id), status TEXT DEFAULT 'pending', PRIMARY KEY (tweet_id, user_id));
CREATE TABLE IF NOT EXISTS messages (id INTEGER PRIMARY KEY AUTOINCREMENT, sender_id INTEGER NOT NULL REFERENCES users(id),
  recipient_id INTEGER NOT NULL REFERENCES users(id), content TEXT NOT NULL, read INTEGER DEFAULT 0, created_at TEXT DEFAULT CURRENT_TIMESTAMP);
)SQL");
    rawExec(R"SQL(
CREATE TABLE IF NOT EXISTS password_resets (user_id INTEGER PRIMARY KEY REFERENCES users(id), code_hash TEXT NOT NULL, expires_at INTEGER NOT NULL, attempts INTEGER DEFAULT 0);
CREATE INDEX IF NOT EXISTS idx_tweets_reply ON tweets(reply_to);
CREATE INDEX IF NOT EXISTS idx_tweets_author ON tweets(author_id);
CREATE INDEX IF NOT EXISTS idx_likes_tweet ON likes(tweet_id);
CREATE INDEX IF NOT EXISTS idx_retweets_tweet ON retweets(tweet_id);
CREATE INDEX IF NOT EXISTS idx_reactions_tweet ON reactions(tweet_id);
CREATE INDEX IF NOT EXISTS idx_coauthors_tweet ON tweet_coauthors(tweet_id);
CREATE INDEX IF NOT EXISTS idx_notif_user ON notifications(user_id, id);
CREATE INDEX IF NOT EXISTS idx_msg_recipient ON messages(recipient_id, read);
CREATE INDEX IF NOT EXISTS idx_msg_sender ON messages(sender_id);
CREATE INDEX IF NOT EXISTS idx_follows_following ON follows(following_id);
)SQL");
    try { rawExec("ALTER TABLE users ADD COLUMN is_guest INTEGER DEFAULT 0"); } catch (const std::exception&) {}  // migration for old DBs
  }
};
