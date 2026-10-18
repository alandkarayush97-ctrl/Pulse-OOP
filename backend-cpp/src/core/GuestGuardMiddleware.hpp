#pragma once
#include <functional>
#include "Http.hpp"

// Blocks write actions for guest accounts (guests are read-only).
class GuestGuardMiddleware final : public IMiddleware {
  std::function<bool(long long)> isGuest_;
 public:
  explicit GuestGuardMiddleware(std::function<bool(long long)> isGuest) : isGuest_(std::move(isGuest)) {}
  bool handle(Request& req, Response& res) override {
    if (isGuest_(req.userId)) { res.status(403).send({{"error", "Guest accounts are read-only. Sign up to do this."}}); return false; }
    return true;
  }
};
