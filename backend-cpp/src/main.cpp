// Pulse backend (C++17) — composition root: wires dependencies, registers controllers, starts the server.
#include <cstdlib>
#include <fstream>
#include <memory>
#include <vector>
#include "controllers/AuthController.hpp"
#include "controllers/MessageController.hpp"
#include "controllers/NotificationController.hpp"
#include "controllers/TweetController.hpp"
#include "controllers/UserController.hpp"
#include "core/Database.hpp"
#include "core/GuestGuardMiddleware.hpp"
#include "core/Http.hpp"
#include "core/Media.hpp"
#include "controllers/MediaController.hpp"
#include "core/Middleware.hpp"

class HealthController final : public Controller {
 public:
  void registerRoutes(Router& r) override { r.get("/api/health", [](Request&, Response& res) { res.send({{"ok", true}}); }); }
};

// Tiny .env reader: KEY=VALUE lines; real environment variables win.
static std::string config(const std::string& key, const std::string& def) {
  if (const char* e = std::getenv(key.c_str())) return e;
  std::ifstream f(".env"); std::string line;
  while (std::getline(f, line)) {
    size_t eq = line.find('=');
    if (eq != std::string::npos && http::trim(line.substr(0, eq)) == key) return http::trim(line.substr(eq + 1));
  }
  return def;
}

int main() {
  std::string secret = config("JWT_SECRET", "");
  if (secret.empty()) { std::cerr << "[warn] JWT_SECRET not set (.env) - using an insecure dev secret\n"; secret = "dev-secret-change-me"; }
  int port = std::atoi(config("PORT", "4000").c_str());

  Media::base() = config("PUBLIC_URL", "http://localhost:" + std::to_string(port));
  Database db(config("DB_PATH", "data.sqlite"));
  db.initSchema();

  // Infrastructure / services
  Pbkdf2Hasher hasher;
  JwtService jwt(secret);
  FactCheckService factCheck;
  MiddlewarePtr auth = std::make_shared<AuthMiddleware>(jwt);
  MiddlewarePtr limiter = std::make_shared<RateLimitMiddleware>(60000, 10);

  // Repositories (all derive from abstract BaseRepository)
  UserRepository users(db);
  TweetRepository tweets(db);
  NotificationRepository notifications(db);
  MessageRepository messages(db);

  MiddlewarePtr writeGuard = std::make_shared<GuestGuardMiddleware>([&users](long long id) { return users.isGuest(id); });
  bool showResetCode = config("SHOW_RESET_CODE", "1") != "0";  // demo mode: no e-mail server

  // Controllers (all derive from abstract Controller) - dependencies injected via constructors
  std::vector<std::unique_ptr<Controller>> controllers;
  controllers.push_back(std::make_unique<AuthController>(users, hasher, jwt, auth, limiter, showResetCode, secret));
  controllers.push_back(std::make_unique<TweetController>(tweets, notifications, users, factCheck, auth, writeGuard));
  controllers.push_back(std::make_unique<UserController>(users, notifications, tweets, auth, writeGuard));
  controllers.push_back(std::make_unique<NotificationController>(notifications, auth));
  controllers.push_back(std::make_unique<MessageController>(messages, auth, writeGuard));
  controllers.push_back(std::make_unique<MediaController>(tweets, users));
  controllers.push_back(std::make_unique<HealthController>());

  Router router;
  for (auto& c : controllers) c->registerRoutes(router);  // polymorphic dispatch

  try { HttpServer(router, port).run(); }
  catch (const std::exception& e) { std::cerr << "Fatal: " << e.what() << "\n"; return 1; }
}
