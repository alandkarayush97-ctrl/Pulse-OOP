#pragma once
#include "../core/Crypto.hpp"
#include "../core/Http.hpp"
#include "../repositories/TweetRepository.hpp"
#include "../repositories/UserRepository.hpp"

// Serves uploaded images/videos/audio as real files (cached) instead of huge base64 inside JSON.
class MediaController final : public Controller {
  TweetRepository& tweets_;
  UserRepository& users_;

  static void serveDataUri(const std::string& uri, Response& res) {
    size_t comma = uri.find(',');
    if (uri.rfind("data:", 0) != 0 || comma == std::string::npos) { res.status(404).send({{"error", "Not found"}}); return; }
    std::string meta = uri.substr(5, comma - 5);                       // e.g. image/png;base64
    std::string mime = meta.substr(0, meta.find(';'));
    res.raw(mime.empty() ? "application/octet-stream" : mime, crypto::b64decode(uri.substr(comma + 1)));
    res.cacheControl = "public, max-age=604800";
  }
  void tweetMedia(Request& req, Response& res) {
    long long id = req.paramId("id");
    auto t = tweets_.getById(id, 0);                                     // viewer 0: hides locked / pending posts
    auto m = tweets_.mediaOf(id);
    if (!t || !t->isVisibleTo(0) || !m) { res.status(404).send({{"error", "Not found"}}); return; }
    serveDataUri(*m, res);
  }
  void avatar(Request& req, Response& res) {
    auto a = users_.avatarOf(req.paramId("id"));
    if (!a) { res.status(404).send({{"error", "Not found"}}); return; }
    serveDataUri(*a, res);
  }
 public:
  MediaController(TweetRepository& t, UserRepository& u) : tweets_(t), users_(u) {}
  void registerRoutes(Router& r) override {
    r.get("/api/media/tweets/:id", HANDLER(tweetMedia));
    r.get("/api/media/avatars/:id", HANDLER(avatar));
  }
};
