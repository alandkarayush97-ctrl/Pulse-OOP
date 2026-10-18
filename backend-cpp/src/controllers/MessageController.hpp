#pragma once
#include "../core/Http.hpp"
#include "../repositories/MessageRepository.hpp"

class MessageController final : public Controller {
  MessageRepository& messages_;
  MiddlewarePtr auth_, write_;
  void listConversations(Request& req, Response& res) { res.send(Json(messages_.getConversations(req.userId))); }
  void getThread(Request& req, Response& res) {
    long long other = req.paramId("userId");
    messages_.markRead(req.userId, other);
    res.send(Json(messages_.getThread(req.userId, other)));
  }
  void unreadCount(Request& req, Response& res) { res.send({{"count", messages_.unreadTotal(req.userId)}}); }
  void send(Request& req, Response& res) {
    std::string content = http::trim(http::str(req.body(), "content"));
    if (content.empty()) { res.status(400).send({{"error", "Content required"}}); return; }
    res.status(201).send(messages_.send(req.userId, req.paramId("userId"), content));
  }
 public:
  MessageController(MessageRepository& m, MiddlewarePtr auth, MiddlewarePtr write) : messages_(m), auth_(std::move(auth)), write_(std::move(write)) {}
  void registerRoutes(Router& r) override {
    const std::vector<MiddlewarePtr> A = {auth_};
    r.get("/api/messages", HANDLER(listConversations), A);
    r.get("/api/messages/unread-count", HANDLER(unreadCount), A);
    r.get("/api/messages/:userId", HANDLER(getThread), A);
    r.post("/api/messages/:userId", HANDLER(send), {auth_, write_});
  }
};
