#pragma once
#include <unordered_map>
#include "Crypto.hpp"
#include "Http.hpp"

// Verifies the Bearer token and stores the user id on the request.
class AuthMiddleware final : public IMiddleware {
  const JwtService& jwt_;
 public:
  explicit AuthMiddleware(const JwtService& jwt) : jwt_(jwt) {}
  bool handle(Request& req, Response& res) override {
    auto it = req.headers.find("authorization");
    if (it == req.headers.end() || it->second.rfind("Bearer ", 0) != 0) { res.status(401).send({{"error", "Missing token"}}); return false; }
    long long id = jwt_.verify(it->second.substr(7));
    if (id < 0) { res.status(401).send({{"error", "Invalid or expired token"}}); return false; }
    req.userId = id;
    return true;
  }
};

// Simple in-memory limiter (per ip+path).
class RateLimitMiddleware final : public IMiddleware {
  struct Entry { int count = 0; long long resetAt = 0; };
  std::unordered_map<std::string, Entry> hits_;
  long long windowMs_; int max_;
 public:
  RateLimitMiddleware(long long windowMs = 60000, int max = 10) : windowMs_(windowMs), max_(max) {}
  bool handle(Request& req, Response& res) override {
    long long now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    Entry& e = hits_[req.ip + req.path];
    if (now > e.resetAt) { e.count = 0; e.resetAt = now + windowMs_; }
    if (++e.count > max_) { res.status(429).send({{"error", "Too many requests, please slow down."}}); return false; }
    return true;
  }
};
