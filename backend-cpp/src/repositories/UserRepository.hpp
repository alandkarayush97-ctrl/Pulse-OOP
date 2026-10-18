#pragma once
#include "BaseRepository.hpp"
#include "../core/Crypto.hpp"

class UserRepository final : public BaseRepository {
 public:
  explicit UserRepository(Database& db) : BaseRepository(db, "users") {}
  std::string entityName() const override { return "User"; }

  std::optional<Json> findByEmail(const std::string& e) { return db_.one("SELECT * FROM users WHERE email = ?", {e}); }
  std::optional<Json> findByHandle(const std::string& h) { return db_.one("SELECT * FROM users WHERE handle = ?", {h}); }

  Json create(const std::string& name, const std::string& handle, const std::string& email, const std::string& hash) {
    long long id = db_.exec("INSERT INTO users (name, handle, email, password) VALUES (?, ?, ?, ?)", {name, handle, email, hash});
    return *findById(id);
  }
  std::optional<std::string> avatarOf(long long id) {
    auto r = db_.one("SELECT avatar_url FROM users WHERE id = ?", {id});
    if (!r || !(*r)["avatar_url"].is_string()) return std::nullopt;
    return (*r)["avatar_url"].get<std::string>();
  }
  long long avatarLength(long long id) { return db_.scalar("SELECT COALESCE(length(avatar_url), 0) FROM users WHERE id = ?", {id}); }

  bool isGuest(long long id) { return db_.scalar("SELECT COALESCE(is_guest, 0) FROM users WHERE id = ?", {id}) != 0; }
  Json ensureGuest(const std::string& passwordHash) {
    if (auto g = findByEmail("guest@pulse.local")) return *g;
    long long id = db_.exec("INSERT INTO users (name, handle, email, password, bio, is_guest) VALUES ('Guest', 'guest', 'guest@pulse.local', ?, 'Browsing as a guest', 1)", {passwordHash});
    return *findById(id);
  }
  void setPassword(long long id, const std::string& hash) { db_.exec("UPDATE users SET password = ? WHERE id = ?", {hash, id}); }

  // --- password reset codes (hashed, expiring, attempt-limited) ---
  void saveResetCode(long long userId, const std::string& codeHash, long long expiresAt) {
    db_.exec("INSERT INTO password_resets (user_id, code_hash, expires_at, attempts) VALUES (?, ?, ?, 0) ON CONFLICT(user_id) DO UPDATE SET code_hash = excluded.code_hash, expires_at = excluded.expires_at, attempts = 0", {userId, codeHash, expiresAt});
  }
  // returns true if code is valid; counts failed attempts (max 5)
  bool consumeResetCode(long long userId, const std::string& codeHash, long long now) {
    auto r = db_.one("SELECT * FROM password_resets WHERE user_id = ?", {userId});
    if (!r) return false;
    if ((*r)["expires_at"].get<long long>() < now || (*r)["attempts"].get<long long>() >= 5) { db_.exec("DELETE FROM password_resets WHERE user_id = ?", {userId}); return false; }
    if ((*r)["code_hash"].get<std::string>() != codeHash) { db_.exec("UPDATE password_resets SET attempts = attempts + 1 WHERE user_id = ?", {userId}); return false; }
    db_.exec("DELETE FROM password_resets WHERE user_id = ?", {userId});
    return true;
  }
  std::vector<Json> search(const std::string& query) {
    std::string q = "%" + query + "%";
    return db_.query("SELECT id, name, handle, bio FROM users WHERE is_guest = 0 AND (name LIKE ? OR handle LIKE ?) LIMIT 20", {q, q});
  }
  long long followerCount(long long id) { return db_.scalar("SELECT COUNT(*) FROM follows WHERE following_id = ?", {id}); }
  long long followingCount(long long id) { return db_.scalar("SELECT COUNT(*) FROM follows WHERE follower_id = ?", {id}); }
  bool isFollowing(long long a, long long b) { return db_.one("SELECT 1 FROM follows WHERE follower_id = ? AND following_id = ?", {a, b}).has_value(); }
  void updateAvatar(long long id, const std::string& url) { db_.exec("UPDATE users SET avatar_url = ? WHERE id = ?", {url, id}); }
  std::vector<Json> listFollowing(long long id) {
    return db_.query("SELECT u.id, u.name, u.handle FROM follows f JOIN users u ON u.id = f.following_id WHERE f.follower_id = ?", {id});
  }
  std::vector<Json> listFollowers(long long id) {
    return db_.query("SELECT u.id, u.name, u.handle FROM follows f JOIN users u ON u.id = f.follower_id WHERE f.following_id = ?", {id});
  }
  std::vector<Json> suggestions(long long exclude, int limit = 3) {
    return db_.query("SELECT id, name, handle FROM users WHERE is_guest = 0 AND id != ? AND id NOT IN (SELECT following_id FROM follows WHERE follower_id = ?) ORDER BY RANDOM() LIMIT ?",
                     {exclude, exclude, limit});
  }
  void follow(long long a, long long b) { db_.exec("INSERT INTO follows (follower_id, following_id) VALUES (?, ?)", {a, b}); }
  void unfollow(long long a, long long b) { db_.exec("DELETE FROM follows WHERE follower_id = ? AND following_id = ?", {a, b}); }
};
