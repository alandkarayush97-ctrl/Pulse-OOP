#pragma once
#include "../core/Http.hpp"
#include "../repositories/NotificationRepository.hpp"
#include "../repositories/TweetRepository.hpp"
#include "../repositories/UserRepository.hpp"
#include "../services/FactCheckService.hpp"

class TweetController final : public Controller {
  TweetRepository& tweets_;
  NotificationRepository& notifications_;
  UserRepository& users_;
  const FactCheckService& factCheck_;
  MiddlewarePtr auth_, write_;

  static Json toArray(const std::vector<Tweet>& v) { Json a = Json::array(); for (const auto& t : v) a.push_back(t.toJson()); return a; }
  static void notFound(Response& res) { res.status(404).send({{"error", "Tweet not found"}}); }

  void getFeed(Request& req, Response& res) {
    long long offset = std::atoll(req.q("offset").c_str());
    res.send(toArray(tweets_.getFeed(req.userId, req.q("vibe"), http::toInt(req.q("limit"), 20), offset < 0 ? 0 : offset)));
  }
  void latestId(Request&, Response& res) { res.send({{"latestId", tweets_.latestId()}}); }
  void search(Request& req, Response& res) { res.send(toArray(tweets_.search(req.q("q"), req.userId))); }
  void trending(Request&, Response& res) { res.send(Json(tweets_.trendingVibes())); }
  void getOne(Request& req, Response& res) {
    auto t = tweets_.getById(req.paramId("id"), req.userId);
    if (!t) return notFound(res);
    res.send(t->toJson());
  }
  void getReplies(Request& req, Response& res) { res.send(toArray(tweets_.getReplies(req.paramId("id"), req.userId))); }

  void create(Request& req, Response& res) {
    const Json& b = req.body();
    std::string content = http::trim(http::str(b, "content"));
    std::string vibe = b.contains("vibe") && b["vibe"].is_string() ? b["vibe"].get<std::string>() : "Wholesome";
    std::string energy = b.contains("energy") && b["energy"].is_string() ? b["energy"].get<std::string>() : "Just sharing";
    if (content.empty()) { res.status(400).send({{"error", "Content required"}}); return; }
    if (std::find(ENERGY_TAGS.begin(), ENERGY_TAGS.end(), energy) == ENERGY_TAGS.end()) { res.status(400).send({{"error", "Invalid energy tag"}}); return; }

    long long coAuthorId = 0;
    std::string handle = http::str(b, "coAuthorHandle");
    if (!handle.empty()) {
      size_t at = handle.find('@'); if (at != std::string::npos) handle.erase(at, 1);
      auto co = users_.findByHandle(http::trim(handle));
      if (!co) { res.status(404).send({{"error", "Co-author handle not found"}}); return; }
      coAuthorId = (*co)["id"];
      if (coAuthorId == req.userId) { res.status(400).send({{"error", "Cannot co-author with yourself"}}); return; }
    }
    std::string pub = http::str(b, "publishAt"), img = http::str(b, "imageUrl");
    Json replyTo = b.contains("replyTo") && b["replyTo"].is_number() ? b["replyTo"] : (b.contains("replyTo") && b["replyTo"].is_string() && !b["replyTo"].get<std::string>().empty() ? Json(std::atoll(b["replyTo"].get<std::string>().c_str())) : Json(nullptr));
    auto tweet = tweets_.create(req.userId, content, vibe, energy, pub.empty() ? Json(nullptr) : Json(pub), replyTo, coAuthorId, img.empty() ? Json(nullptr) : Json(img));
    if (coAuthorId) notifications_.create(coAuthorId, "coauthor_invite", req.userId, tweet->id());
    if (!replyTo.is_null()) {
      auto parent = tweets_.getById(replyTo, req.userId);
      if (parent && parent->authorId() != req.userId) notifications_.create(parent->authorId(), "reply", req.userId, replyTo);
    }
    res.status(201).send(tweet->toJson());
  }
  void update(Request& req, Response& res) {
    long long id = req.paramId("id");
    auto t = tweets_.getById(id, req.userId);
    if (!t) return notFound(res);
    if (t->authorId() != req.userId) { res.status(403).send({{"error", "Not your post"}}); return; }
    std::string content = http::trim(http::str(req.body(), "content"));
    if (content.empty()) { res.status(400).send({{"error", "Content required"}}); return; }
    tweets_.update(id, content);
    res.send(tweets_.getById(id, req.userId)->toJson());
  }
  void remove(Request& req, Response& res) {
    long long id = req.paramId("id");
    auto t = tweets_.getById(id, req.userId);
    if (!t) return notFound(res);
    if (t->authorId() != req.userId) { res.status(403).send({{"error", "Not your post"}}); return; }
    tweets_.remove(id);  // polymorphic call -> TweetRepository::remove (cascade)
    res.send({{"ok", true}});
  }
  void respondCoauthor(Request& req, Response& res) {
    const Json& b = req.body();
    tweets_.respondToCoauthor(req.paramId("id"), req.userId, b.contains("accept") && http::truthy(b["accept"]));
    res.send({{"ok", true}});
  }
  void factCheck(Request& req, Response& res) {
    auto t = tweets_.getById(req.paramId("id"), req.userId);
    if (!t) return notFound(res);
    res.send(factCheck_.analyze(t->content()));
  }
  void toggleInteraction(Request& req, Response& res, bool isLike) {
    long long id = req.paramId("id");
    bool on = isLike ? tweets_.toggleLike(req.userId, id) : tweets_.toggleRetweet(req.userId, id);
    if (on) {
      auto t = tweets_.getById(id, req.userId);
      if (t && t->authorId() != req.userId) notifications_.create(t->authorId(), isLike ? "like" : "retweet", req.userId, id);
    }
    res.send({{isLike ? "liked" : "retweeted", on}});
  }
  void react(Request& req, Response& res) {
    std::string emoji = http::str(req.body(), "emoji");
    if (emoji.empty()) { res.status(400).send({{"error", "Emoji required"}}); return; }
    res.send({{"myReaction", tweets_.setReaction(req.userId, req.paramId("id"), emoji)}});
  }
 public:
  TweetController(TweetRepository& t, NotificationRepository& n, UserRepository& u, const FactCheckService& f, MiddlewarePtr auth, MiddlewarePtr write)
      : tweets_(t), notifications_(n), users_(u), factCheck_(f), auth_(std::move(auth)), write_(std::move(write)) {}

  void registerRoutes(Router& r) override {
    const std::vector<MiddlewarePtr> A = {auth_}, W = {auth_, write_};
    r.get("/api/tweets", HANDLER(getFeed), A);
    r.get("/api/tweets/latest-id", HANDLER(latestId), A);
    r.get("/api/tweets/search", HANDLER(search), A);
    r.get("/api/tweets/trending", HANDLER(trending), A);
    r.get("/api/tweets/:id", HANDLER(getOne), A);
    r.get("/api/tweets/:id/replies", HANDLER(getReplies), A);
    r.post("/api/tweets", HANDLER(create), W);
    r.patch("/api/tweets/:id", HANDLER(update), W);
    r.del("/api/tweets/:id", HANDLER(remove), W);
    r.post("/api/tweets/:id/like", [this](Request& q, Response& s) { toggleInteraction(q, s, true); }, W);
    r.post("/api/tweets/:id/retweet", [this](Request& q, Response& s) { toggleInteraction(q, s, false); }, W);
    r.post("/api/tweets/:id/react", HANDLER(react), W);
    r.post("/api/tweets/:id/coauthor/respond", HANDLER(respondCoauthor), W);
    r.post("/api/tweets/:id/factcheck", HANDLER(factCheck), A);
  }
};
