#pragma once
#include "../core/Http.hpp"
#include "../repositories/NotificationRepository.hpp"

class NotificationController final : public Controller {
  NotificationRepository& notifications_;
  MiddlewarePtr auth_;
  void list(Request& req, Response& res) {
    Json arr = Json::array();
    for (const auto& n : notifications_.getForUser(req.userId)) arr.push_back(n->toJson());  // virtual getMessage()
    res.send(arr);
  }
  void markRead(Request& req, Response& res) { notifications_.markAllRead(req.userId); res.send({{"ok", true}}); }
 public:
  NotificationController(NotificationRepository& n, MiddlewarePtr auth) : notifications_(n), auth_(std::move(auth)) {}
  void registerRoutes(Router& r) override {
    r.get("/api/notifications", HANDLER(list), {auth_});
    r.post("/api/notifications/read", HANDLER(markRead), {auth_});
  }
};
