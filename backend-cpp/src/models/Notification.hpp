#pragma once
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include "../core/Database.hpp"

// Abstract base: cannot be instantiated (pure virtual getMessage).
class Notification {
 protected:
  long long id_, actorId_;
  std::string type_, createdAt_, actorName_, actorHandle_;
  bool read_;
  Json tweetId_;
  explicit Notification(const Json& r)
      : id_(r["id"]), actorId_(r["actor_id"]), type_(r["type"]), createdAt_(r.value("created_at", "")),
        actorName_(r["actor_name"]), actorHandle_(r["actor_handle"]), read_(r["read"].get<long long>() != 0), tweetId_(r["tweet_id"]) {}
 public:
  virtual ~Notification() = default;
  virtual std::string getMessage() const = 0;  // overridden by each subclass -> polymorphism

  Json toJson() const {  // template-method: shared structure, subclass-specific message
    return {{"id", id_}, {"type", type_}, {"read", read_}, {"createdAt", createdAt_},
            {"actor", {{"id", actorId_}, {"name", actorName_}, {"handle", actorHandle_}}},
            {"tweetId", tweetId_}, {"message", getMessage()}};
  }
};

class LikeNotification final : public Notification {
 public: explicit LikeNotification(const Json& r) : Notification(r) {}
  std::string getMessage() const override { return actorName_ + " liked your post"; }
};
class RetweetNotification final : public Notification {
 public: explicit RetweetNotification(const Json& r) : Notification(r) {}
  std::string getMessage() const override { return actorName_ + " reposted your post"; }
};
class ReplyNotification final : public Notification {
 public: explicit ReplyNotification(const Json& r) : Notification(r) {}
  std::string getMessage() const override { return actorName_ + " replied to your post"; }
};
class FollowNotification final : public Notification {
 public: explicit FollowNotification(const Json& r) : Notification(r) {}
  std::string getMessage() const override { return actorName_ + " followed you"; }
};
class CoauthorNotification final : public Notification {
 public: explicit CoauthorNotification(const Json& r) : Notification(r) {}
  std::string getMessage() const override { return actorName_ + " invited you to co-author a post"; }
};

// Factory: callers never branch on `type`.
class NotificationFactory {
  using Maker = std::function<std::unique_ptr<Notification>(const Json&)>;
  static const std::map<std::string, Maker>& registry() {
    static const std::map<std::string, Maker> r = {
      {"like", [](const Json& j) { return std::make_unique<LikeNotification>(j); }},
      {"retweet", [](const Json& j) { return std::make_unique<RetweetNotification>(j); }},
      {"reply", [](const Json& j) { return std::make_unique<ReplyNotification>(j); }},
      {"follow", [](const Json& j) { return std::make_unique<FollowNotification>(j); }},
      {"coauthor_invite", [](const Json& j) { return std::make_unique<CoauthorNotification>(j); }},
    };
    return r;
  }
 public:
  static std::unique_ptr<Notification> create(const Json& row) {
    auto it = registry().find(row["type"].get<std::string>());
    if (it == registry().end()) throw std::runtime_error("Unknown notification type");
    return it->second(row);
  }
};
