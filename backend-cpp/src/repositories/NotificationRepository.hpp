#pragma once
#include <memory>
#include "BaseRepository.hpp"
#include "../models/Notification.hpp"

class NotificationRepository final : public BaseRepository {
 public:
  explicit NotificationRepository(Database& db) : BaseRepository(db, "notifications") {}
  std::string entityName() const override { return "Notification"; }

  std::vector<std::unique_ptr<Notification>> getForUser(long long userId) {
    auto rows = db_.query("SELECT n.*, u.name AS actor_name, u.handle AS actor_handle FROM notifications n JOIN users u ON u.id = n.actor_id WHERE n.user_id = ? ORDER BY n.id DESC LIMIT 50", {userId});
    std::vector<std::unique_ptr<Notification>> out;
    for (const auto& r : rows) out.push_back(NotificationFactory::create(r));  // polymorphism via factory
    return out;
  }
  void create(long long userId, const std::string& type, long long actorId, Json tweetId = nullptr) {
    db_.exec("INSERT INTO notifications (user_id, type, actor_id, tweet_id) VALUES (?, ?, ?, ?)", {userId, type, actorId, tweetId});
  }
  void markAllRead(long long userId) { db_.exec("UPDATE notifications SET read = 1 WHERE user_id = ?", {userId}); }
};
