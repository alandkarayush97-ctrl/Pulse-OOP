#pragma once
#include "../core/Http.hpp"
#include "../core/Media.hpp"
#include "../repositories/NotificationRepository.hpp"
#include "../repositories/TweetRepository.hpp"
#include "../repositories/UserRepository.hpp"

class UserController final : public Controller {
  UserRepository& users_;
  NotificationRepository& notifications_;
  TweetRepository& tweets_;
  MiddlewarePtr auth_, write_;

  bool findUser(Request& req, Response& res, Json& out) {
    auto u = users_.findByHandle(req.params["handle"]);
    if (!u) { res.status(404).send({{"error", "User not found"}}); return false; }
    out = *u; return true;
  }
  void updateAvatar(Request& req, Response& res) {
    std::string url = http::str(req.body(), "avatarUrl");
    if (url.empty()) { res.status(400).send({{"error", "avatarUrl required"}}); return; }
    users_.updateAvatar(req.userId, url);
    res.send({{"avatarUrl", url}});
  }
  void getTweets(Request& req, Response& res) {
    Json u; if (!findUser(req, res, u)) return;
    Json arr = Json::array();
    for (const auto& t : tweets_.getByAuthor(u["id"], req.userId)) arr.push_back(t.toJson());
    res.send(arr);
  }
  void getFollowing(Request& req, Response& res) { Json u; if (findUser(req, res, u)) res.send(Json(users_.listFollowing(u["id"]))); }
  void getFollowers(Request& req, Response& res) { Json u; if (findUser(req, res, u)) res.send(Json(users_.listFollowers(u["id"]))); }
  void suggestions(Request& req, Response& res) { res.send(Json(users_.suggestions(req.userId))); }
  void search(Request& req, Response& res) { res.send(Json(users_.search(req.q("q")))); }
  void getProfile(Request& req, Response& res) {
    Json u; if (!findUser(req, res, u)) return;
    long long id = u["id"];
    res.send({{"id", id}, {"name", u["name"]}, {"handle", u["handle"]}, {"bio", u["bio"]}, {"avatarUrl", u["avatar_url"].is_string() ? Json(Media::base() + "/api/media/avatars/" + std::to_string(id) + "?v=" + std::to_string(users_.avatarLength(id))) : Json(nullptr)},
              {"createdAt", u["created_at"]}, {"followers", users_.followerCount(id)}, {"following", users_.followingCount(id)},
              {"isFollowing", users_.isFollowing(req.userId, id)}, {"isMe", id == req.userId}});
  }
  void toggleFollow(Request& req, Response& res) {
    Json t; if (!findUser(req, res, t)) return;
    long long id = t["id"];
    if (id == req.userId) { res.status(400).send({{"error", "Cannot follow yourself"}}); return; }
    bool already = users_.isFollowing(req.userId, id);
    if (already) users_.unfollow(req.userId, id);
    else { users_.follow(req.userId, id); notifications_.create(id, "follow", req.userId); }
    res.send({{"following", !already}});
  }
 public:
  UserController(UserRepository& u, NotificationRepository& n, TweetRepository& t, MiddlewarePtr auth, MiddlewarePtr write)
      : users_(u), notifications_(n), tweets_(t), auth_(std::move(auth)), write_(std::move(write)) {}
  void registerRoutes(Router& r) override {
    const std::vector<MiddlewarePtr> A = {auth_};
    r.get("/api/users/search", HANDLER(search), A);
    r.get("/api/users/suggestions", HANDLER(suggestions), A);
    r.post("/api/users/me/avatar", HANDLER(updateAvatar), {auth_, write_});
    r.get("/api/users/:handle", HANDLER(getProfile), A);
    r.get("/api/users/:handle/tweets", HANDLER(getTweets), A);
    r.get("/api/users/:handle/following", HANDLER(getFollowing), A);
    r.get("/api/users/:handle/followers", HANDLER(getFollowers), A);
    r.post("/api/users/:handle/follow", HANDLER(toggleFollow), {auth_, write_});
  }
};
