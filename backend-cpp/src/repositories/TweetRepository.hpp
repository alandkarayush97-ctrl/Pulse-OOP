#pragma once
#include <vector>
#include "BaseRepository.hpp"
#include "../models/Tweet.hpp"

class TweetRepository final : public BaseRepository {
  static constexpr const char* SELECT =
      "SELECT t.id, t.author_id, t.content, t.vibe, t.energy, t.publish_at, t.reply_to, t.created_at, "
      "CASE WHEN t.image_url IS NULL THEN NULL ELSE substr(t.image_url, 1, 40) END AS image_url, length(t.image_url) AS image_len, "
      "u.name AS author_name, u.handle AS author_handle FROM tweets t JOIN users u ON u.id = t.author_id ";

  Tweet hydrate(const Json& row, long long viewer) {
    long long id = row["id"];
    auto my = db_.one("SELECT emoji FROM reactions WHERE user_id = ? AND tweet_id = ?", {viewer, id});
    return Tweet(row,
      db_.scalar("SELECT COUNT(*) FROM likes WHERE tweet_id = ?", {id}),
      db_.scalar("SELECT COUNT(*) FROM retweets WHERE tweet_id = ?", {id}),
      db_.scalar("SELECT COUNT(*) FROM tweets WHERE reply_to = ?", {id}),
      db_.one("SELECT 1 FROM likes WHERE user_id = ? AND tweet_id = ?", {viewer, id}).has_value(),
      Json(getReactionSummary(id)), my ? (*my)["emoji"] : Json(nullptr), Json(getCoAuthors(id)));
  }
  std::vector<Tweet> visible(const std::vector<Json>& rows, long long viewer) {
    std::vector<Tweet> out;
    for (const auto& r : rows) { Tweet t = hydrate(r, viewer); if (t.isVisibleTo(viewer)) out.push_back(std::move(t)); }
    return out;
  }
 public:
  explicit TweetRepository(Database& db) : BaseRepository(db, "tweets") {}
  std::string entityName() const override { return "Tweet"; }

  std::vector<Json> getCoAuthors(long long id) {
    return db_.query("SELECT u.id, u.name, u.handle, tc.status FROM tweet_coauthors tc JOIN users u ON u.id = tc.user_id WHERE tc.tweet_id = ?", {id});
  }
  std::vector<Json> getReactionSummary(long long id) {
    return db_.query("SELECT emoji, COUNT(*) count FROM reactions WHERE tweet_id = ? GROUP BY emoji", {id});
  }
  std::vector<Tweet> getFeed(long long viewer, const std::string& vibe, long long limit, long long offset) {
    if (!vibe.empty() && vibe != "All")
      return visible(db_.query(std::string(SELECT) + "WHERE t.reply_to IS NULL AND t.vibe = ? ORDER BY t.id DESC LIMIT ? OFFSET ?", {vibe, limit, offset}), viewer);
    return visible(db_.query(std::string(SELECT) + "WHERE t.reply_to IS NULL ORDER BY t.id DESC LIMIT ? OFFSET ?", {limit, offset}), viewer);
  }
  long long latestId() { return db_.scalar("SELECT COALESCE(MAX(id), 0) FROM tweets WHERE reply_to IS NULL"); }
  std::vector<Tweet> search(const std::string& q, long long viewer) {
    return visible(db_.query(std::string(SELECT) + "WHERE t.content LIKE ? AND t.reply_to IS NULL ORDER BY t.id DESC LIMIT 30", {"%" + q + "%"}), viewer);
  }
  std::vector<Tweet> getReplies(long long id, long long viewer) {
    return visible(db_.query(std::string(SELECT) + "WHERE t.reply_to = ? ORDER BY t.id ASC", {id}), viewer);
  }
  std::vector<Tweet> getByAuthor(long long author, long long viewer) {
    return visible(db_.query(std::string(SELECT) + "WHERE t.author_id = ? AND t.reply_to IS NULL ORDER BY t.id DESC", {author}), viewer);
  }
  std::optional<Tweet> getById(long long id, long long viewer) {
    auto row = db_.one(std::string(SELECT) + "WHERE t.id = ?", {id});
    if (!row) return std::nullopt;
    return hydrate(*row, viewer);
  }
  std::optional<Tweet> create(long long author, const std::string& content, const std::string& vibe, const std::string& energy,
                              Json publishAt, Json replyTo, long long coAuthorId, Json imageUrl) {
    long long id = db_.exec("INSERT INTO tweets (author_id, content, vibe, energy, publish_at, image_url, reply_to) VALUES (?, ?, ?, ?, ?, ?, ?)",
                            {author, content, vibe, energy, publishAt, imageUrl, replyTo});
    if (coAuthorId) db_.exec("INSERT INTO tweet_coauthors (tweet_id, user_id, status) VALUES (?, ?, 'pending')", {id, coAuthorId});
    return getById(id, author);
  }
  void update(long long id, const std::string& content) { db_.exec("UPDATE tweets SET content = ? WHERE id = ?", {content, id}); }
  void respondToCoauthor(long long tweet, long long user, bool accept) {
    db_.exec("UPDATE tweet_coauthors SET status = ? WHERE tweet_id = ? AND user_id = ?", {accept ? "accepted" : "declined", tweet, user});
  }
  bool toggle(const char* table, long long user, long long tweet) {
    std::string t = table;
    if (db_.one("SELECT 1 FROM " + t + " WHERE user_id = ? AND tweet_id = ?", {user, tweet})) {
      db_.exec("DELETE FROM " + t + " WHERE user_id = ? AND tweet_id = ?", {user, tweet}); return false;
    }
    db_.exec("INSERT INTO " + t + " (user_id, tweet_id) VALUES (?, ?)", {user, tweet}); return true;
  }
  bool toggleLike(long long u, long long t) { return toggle("likes", u, t); }
  bool toggleRetweet(long long u, long long t) { return toggle("retweets", u, t); }

  // Overrides the base remove(): cascades into replies and dependent rows.
  void remove(long long id) override {
    for (const auto& r : db_.query("SELECT id FROM tweets WHERE reply_to = ?", {id})) remove(r["id"].get<long long>());
    for (const char* t : {"likes", "retweets", "reactions", "tweet_coauthors", "notifications"})
      db_.exec(std::string("DELETE FROM ") + t + " WHERE tweet_id = ?", {id});
    BaseRepository::remove(id);
  }
  Json setReaction(long long user, long long tweet, const std::string& emoji) {
    auto ex = db_.one("SELECT emoji FROM reactions WHERE user_id = ? AND tweet_id = ?", {user, tweet});
    if (ex && (*ex)["emoji"] == emoji) { db_.exec("DELETE FROM reactions WHERE user_id = ? AND tweet_id = ?", {user, tweet}); return nullptr; }
    db_.exec("INSERT INTO reactions (user_id, tweet_id, emoji) VALUES (?, ?, ?) ON CONFLICT(user_id, tweet_id) DO UPDATE SET emoji = excluded.emoji", {user, tweet, emoji});
    return emoji;
  }
  std::optional<std::string> mediaOf(long long id) {
    auto r = db_.one("SELECT image_url FROM tweets WHERE id = ?", {id});
    if (!r || !(*r)["image_url"].is_string()) return std::nullopt;
    return (*r)["image_url"].get<std::string>();
  }
  std::vector<Json> trendingVibes() {
    return db_.query("SELECT vibe, COUNT(*) count FROM tweets WHERE reply_to IS NULL GROUP BY vibe ORDER BY count DESC LIMIT 5");
  }
};
