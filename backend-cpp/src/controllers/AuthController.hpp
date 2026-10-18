#pragma once
#include <cctype>
#include <ctime>
#include <iostream>
#include <random>
#include "../core/Crypto.hpp"
#include "../core/Http.hpp"
#include "../core/Middleware.hpp"
#include "../repositories/UserRepository.hpp"

class AuthController final : public Controller {
  UserRepository& users_;
  const IPasswordHasher& hasher_;  // depends on the abstraction (DI)
  const JwtService& jwt_;
  MiddlewarePtr auth_, limiter_;
  bool showResetCode_;
  std::string secret_;

  static Json publicUser(const Json& u) {
    return {{"id", u["id"]}, {"name", u["name"]}, {"handle", u["handle"]}, {"email", u["email"]}, {"bio", u["bio"]},
            {"isGuest", u.contains("is_guest") && u["is_guest"].is_number() && u["is_guest"].get<long long>() != 0}};
  }
  void signup(Request& req, Response& res) {
    const Json& b = req.body();
    std::string name = http::str(b, "name"), email = http::str(b, "email"), pw = http::str(b, "password");
    if (name.empty() || email.empty() || pw.empty()) { res.status(400).send({{"error", "Missing fields"}}); return; }
    std::string raw = http::str(b, "handle"); if (raw.empty()) raw = name;
    std::string handle;
    for (unsigned char c : raw) { c = static_cast<unsigned char>(std::tolower(c)); if ((std::isalnum(c) && c < 128) || c == '_') handle += static_cast<char>(c); }
    try {
      Json user = users_.create(name, handle, email, hasher_.hash(pw));
      res.send({{"token", jwt_.sign(user["id"])}, {"user", publicUser(user)}});
    } catch (const std::runtime_error&) { res.status(409).send({{"error", "Email or handle already taken"}}); }
  }
  void login(Request& req, Response& res) {
    auto user = users_.findByEmail(http::str(req.body(), "email"));
    if (!user || !hasher_.verify(http::str(req.body(), "password"), (*user)["password"])) { res.status(401).send({{"error", "Invalid credentials"}}); return; }
    res.send({{"token", jwt_.sign((*user)["id"])}, {"user", publicUser(*user)}});
  }
  std::string codeHash(const std::string& code, long long userId) const {
    return crypto::hex(crypto::sha256(crypto::toBytes(code + ":" + std::to_string(userId) + ":" + secret_)));
  }
  // Forgot password: creates a 6-digit code valid 15 min. (No e-mail server in this project, so the code is
  // printed in the server console and - in demo mode - returned in the response.)
  void forgot(Request& req, Response& res) {
    std::string email = http::str(req.body(), "email");
    Json out = {{"ok", true}, {"message", "If that email exists, a reset code has been generated."}};
    auto user = users_.findByEmail(email);
    if (user && !(*user)["is_guest"].is_null() && (*user)["is_guest"].get<long long>() == 0) {
      std::random_device rd;
      std::string code = std::to_string(100000 + rd() % 900000);
      long long id = (*user)["id"];
      users_.saveResetCode(id, codeHash(code, id), static_cast<long long>(std::time(nullptr)) + 15 * 60);
      std::cout << "[reset] code for " << email << ": " << code << " (valid 15 min)" << std::endl;
      if (showResetCode_) out["code"] = code;
    }
    res.send(out);
  }
  void reset(Request& req, Response& res) {
    const Json& b = req.body();
    std::string code = http::str(b, "code"), pw = http::str(b, "password");
    if (pw.size() < 6) { res.status(400).send({{"error", "Password must be at least 6 characters"}}); return; }
    auto user = users_.findByEmail(http::str(b, "email"));
    if (!user || code.empty() || !users_.consumeResetCode((*user)["id"], codeHash(code, (*user)["id"]), static_cast<long long>(std::time(nullptr)))) {
      res.status(400).send({{"error", "Invalid or expired reset code"}}); return;
    }
    users_.setPassword((*user)["id"], hasher_.hash(pw));
    res.send({{"ok", true}});
  }
  // Guest account: shared read-only user, short-lived token.
  void guest(Request&, Response& res) {
    std::random_device rd; std::string junk; for (int i = 0; i < 4; ++i) junk += std::to_string(rd());
    Json user = users_.ensureGuest(hasher_.hash(junk));
    res.send({{"token", jwt_.sign(user["id"], 12 * 3600)}, {"user", publicUser(user)}});
  }
  void me(Request& req, Response& res) {
    auto user = users_.findById(req.userId);
    if (!user) { res.status(401).send({{"error", "Invalid or expired token"}}); return; }
    res.send(publicUser(*user));
  }
 public:
  AuthController(UserRepository& u, const IPasswordHasher& h, const JwtService& j, MiddlewarePtr auth, MiddlewarePtr limiter,
                 bool showResetCode, std::string secret)
      : users_(u), hasher_(h), jwt_(j), auth_(std::move(auth)), limiter_(std::move(limiter)), showResetCode_(showResetCode), secret_(std::move(secret)) {}
  void registerRoutes(Router& r) override {
    r.post("/api/auth/signup", HANDLER(signup), {limiter_});
    r.post("/api/auth/login", HANDLER(login), {limiter_});
    r.post("/api/auth/forgot", HANDLER(forgot), {limiter_});
    r.post("/api/auth/reset", HANDLER(reset), {limiter_});
    r.post("/api/auth/guest", HANDLER(guest), {limiter_});
    r.get("/api/auth/me", HANDLER(me), {auth_});
  }
};
