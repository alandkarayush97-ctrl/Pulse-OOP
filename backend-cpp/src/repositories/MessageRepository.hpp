#pragma once
#include "BaseRepository.hpp"

class MessageRepository final : public BaseRepository {
 public:
  explicit MessageRepository(Database& db) : BaseRepository(db, "messages") {}
  std::string entityName() const override { return "Message"; }

  std::vector<Json> getConversations(long long u) {
    return db_.query(R"SQL(SELECT u.id, u.name, u.handle,
      (SELECT content FROM messages m2 WHERE (m2.sender_id = u.id AND m2.recipient_id = ?) OR (m2.sender_id = ? AND m2.recipient_id = u.id) ORDER BY m2.id DESC LIMIT 1) AS last_message,
      (SELECT COUNT(*) FROM messages m3 WHERE m3.sender_id = u.id AND m3.recipient_id = ? AND m3.read = 0) AS unread_count
      FROM users u WHERE u.id IN (SELECT sender_id FROM messages WHERE recipient_id = ? UNION SELECT recipient_id FROM messages WHERE sender_id = ?))SQL",
      {u, u, u, u, u});
  }
  std::vector<Json> getThread(long long u, long long o) {
    return db_.query("SELECT * FROM messages WHERE (sender_id = ? AND recipient_id = ?) OR (sender_id = ? AND recipient_id = ?) ORDER BY id ASC", {u, o, o, u});
  }
  void markRead(long long u, long long o) { db_.exec("UPDATE messages SET read = 1 WHERE sender_id = ? AND recipient_id = ?", {o, u}); }
  long long unreadTotal(long long u) { return db_.scalar("SELECT COUNT(*) FROM messages WHERE recipient_id = ? AND read = 0", {u}); }
  Json send(long long s, long long r, const std::string& content) {
    long long id = db_.exec("INSERT INTO messages (sender_id, recipient_id, content) VALUES (?, ?, ?)", {s, r, content});
    return *findById(id);
  }
};
