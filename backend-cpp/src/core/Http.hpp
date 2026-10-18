#pragma once
// Minimal HTTP framework: Request, Response, abstract Middleware, Router, abstract Controller, HttpServer.
#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
  #endif
  #ifndef NOMINMAX
  #define NOMINMAX
  #endif
  #include <winsock2.h>
  #include <ws2tcpip.h>
  using sock_t = SOCKET;
  #define CLOSESOCK closesocket
  #define BAD_SOCK INVALID_SOCKET
#else
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <unistd.h>
  using sock_t = int;
  #define CLOSESOCK ::close
  #define BAD_SOCK (-1)
#endif
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include "Database.hpp"  // Json alias

// ---------- helpers ----------
namespace http {
inline std::string urlDecode(const std::string& s) {
  std::string o;
  for (size_t i = 0; i < s.size(); ++i) {
    if (s[i] == '+') o += ' ';
    else if (s[i] == '%' && i + 2 < s.size() + 0 && std::isxdigit((unsigned char)s[i+1]) && std::isxdigit((unsigned char)s[i+2])) {
      o += static_cast<char>(std::stoi(s.substr(i + 1, 2), nullptr, 16)); i += 2;
    } else o += s[i];
  }
  return o;
}
inline std::string trim(const std::string& s) {
  size_t a = s.find_first_not_of(" \t\r\n"), b = s.find_last_not_of(" \t\r\n");
  return a == std::string::npos ? "" : s.substr(a, b - a + 1);
}
inline std::string lower(std::string s) { for (auto& c : s) c = static_cast<char>(std::tolower((unsigned char)c)); return s; }
inline long long toInt(const std::string& s, long long def) {
  char* end = nullptr; long long v = std::strtoll(s.c_str(), &end, 10);
  return (end == s.c_str() || v == 0) ? def : v;
}
inline bool truthy(const Json& v) {
  if (v.is_null()) return false;
  if (v.is_boolean()) return v.get<bool>();
  if (v.is_number()) return v.get<double>() != 0;
  if (v.is_string()) return !v.get<std::string>().empty();
  return true;
}
inline std::string str(const Json& body, const char* key) {  // "" when missing / not a string
  return body.contains(key) && body[key].is_string() ? body[key].get<std::string>() : "";
}
}  // namespace http

// ---------- Request / Response ----------
class Request {
  mutable Json body_;
  mutable bool parsed_ = false;
 public:
  std::string method, path, rawBody, ip;
  std::map<std::string, std::string> query, headers, params;
  long long userId = 0;  // set by AuthMiddleware

  const Json& body() const {
    if (!parsed_) { body_ = Json::parse(rawBody, nullptr, false); if (!body_.is_object()) body_ = Json::object(); parsed_ = true; }
    return body_;
  }
  std::string q(const std::string& k, const std::string& def = "") const { auto it = query.find(k); return it == query.end() ? def : it->second; }
  long long paramId(const std::string& k) const { auto it = params.find(k); return it == params.end() ? 0 : std::atoll(it->second.c_str()); }
};

class Response {
 public:
  int code = 200;
  std::string payload = "{}";
  std::string contentType = "application/json; charset=utf-8";
  std::string cacheControl;  // optional
  Response& raw(const std::string& type, std::string body) { contentType = type; payload = std::move(body); return *this; }
  Response& status(int c) { code = c; return *this; }
  Response& send(const Json& j) { payload = j.dump(-1, ' ', false, Json::error_handler_t::replace); return *this; }
};

// ---------- Middleware (abstract) ----------
class IMiddleware {
 public:
  virtual ~IMiddleware() = default;
  virtual bool handle(Request& req, Response& res) = 0;  // false => stop chain
};
using MiddlewarePtr = std::shared_ptr<IMiddleware>;
using Handler = std::function<void(Request&, Response&)>;

// ---------- Router ----------
class Router {
  struct Route { std::string method; std::vector<std::string> segs; std::vector<MiddlewarePtr> mw; Handler fn; };
  std::vector<Route> routes_;

  static std::vector<std::string> split(const std::string& p) {
    std::vector<std::string> out; std::stringstream ss(p); std::string s;
    while (std::getline(ss, s, '/')) if (!s.empty()) out.push_back(s);
    return out;
  }
  void add(const std::string& m, const std::string& path, Handler fn, std::vector<MiddlewarePtr> mw) {
    routes_.push_back({m, split(path), std::move(mw), std::move(fn)});
  }
 public:
  void get(const std::string& p, Handler h, std::vector<MiddlewarePtr> mw = {}) { add("GET", p, std::move(h), std::move(mw)); }
  void post(const std::string& p, Handler h, std::vector<MiddlewarePtr> mw = {}) { add("POST", p, std::move(h), std::move(mw)); }
  void patch(const std::string& p, Handler h, std::vector<MiddlewarePtr> mw = {}) { add("PATCH", p, std::move(h), std::move(mw)); }
  void del(const std::string& p, Handler h, std::vector<MiddlewarePtr> mw = {}) { add("DELETE", p, std::move(h), std::move(mw)); }

  void dispatch(Request& req, Response& res) const {
    auto segs = split(req.path);
    for (const auto& r : routes_) {
      if (r.method != req.method || r.segs.size() != segs.size()) continue;
      std::map<std::string, std::string> params; bool ok = true;
      for (size_t i = 0; i < segs.size() && ok; ++i) {
        if (r.segs[i][0] == ':') params[r.segs[i].substr(1)] = http::urlDecode(segs[i]);
        else ok = r.segs[i] == segs[i];
      }
      if (!ok) continue;
      req.params = params;
      for (const auto& m : r.mw) if (!m->handle(req, res)) return;
      r.fn(req, res);
      return;
    }
    res.status(404).send({{"error", "Not found"}});
  }
};

// ---------- Controller (abstract) ----------
class Controller {
 public:
  virtual ~Controller() = default;
  virtual void registerRoutes(Router& router) = 0;  // each subclass declares its own routes
};
#define HANDLER(fn) [this](Request& rq, Response& rs) { fn(rq, rs); }

// ---------- HttpServer ----------
class HttpServer {
  const Router& router_;
  int port_;
  std::mutex mu_;  // serialises request handling (single SQLite connection)
  static constexpr size_t kMaxBody = 15u * 1024 * 1024;

  static void sendAll(sock_t s, const std::string& d) {
    size_t off = 0;
    while (off < d.size()) {
      int n = static_cast<int>(::send(s, d.data() + off, static_cast<int>(std::min<size_t>(d.size() - off, 1 << 20)), 0));
      if (n <= 0) return;
      off += static_cast<size_t>(n);
    }
  }
  static const char* reason(int c) {
    switch (c) { case 200: return "OK"; case 201: return "Created"; case 204: return "No Content"; case 400: return "Bad Request";
      case 401: return "Unauthorized"; case 403: return "Forbidden"; case 404: return "Not Found"; case 409: return "Conflict";
      case 413: return "Payload Too Large"; case 429: return "Too Many Requests"; default: return "Internal Server Error"; }
  }
  static void reply(sock_t s, int code, const std::string& body, const std::string& type = "application/json; charset=utf-8", const std::string& cache = "") {
    std::ostringstream o;
    o << "HTTP/1.1 " << code << ' ' << reason(code) << "\r\nContent-Type: " << type << "\r\n"
      << (cache.empty() ? "" : "Cache-Control: " + cache + "\r\n")
      << "Access-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: Content-Type, Authorization\r\n"
      << "Access-Control-Allow-Methods: GET,POST,PATCH,PUT,DELETE,OPTIONS\r\nContent-Length: " << body.size()
      << "\r\nConnection: close\r\n\r\n" << body;
    sendAll(s, o.str());
  }

  void serve(sock_t s, std::string ip) {
    std::string buf; char tmp[16384]; size_t he;
    while ((he = buf.find("\r\n\r\n")) == std::string::npos) {
      int n = static_cast<int>(::recv(s, tmp, sizeof tmp, 0));
      if (n <= 0 || buf.size() > 65536) { CLOSESOCK(s); return; }
      buf.append(tmp, static_cast<size_t>(n));
    }
    Request req; req.ip = ip;
    std::istringstream head(buf.substr(0, he));
    std::string line, target, version;
    std::getline(head, line);
    std::istringstream rl(line); rl >> req.method >> target >> version;
    while (std::getline(head, line)) {
      size_t c = line.find(':');
      if (c != std::string::npos) req.headers[http::lower(line.substr(0, c))] = http::trim(line.substr(c + 1));
    }
    size_t qm = target.find('?');
    req.path = http::urlDecode(target.substr(0, qm));
    if (qm != std::string::npos) {
      std::stringstream qs(target.substr(qm + 1)); std::string kv;
      while (std::getline(qs, kv, '&')) {
        size_t eq = kv.find('=');
        req.query[http::urlDecode(kv.substr(0, eq))] = eq == std::string::npos ? "" : http::urlDecode(kv.substr(eq + 1));
      }
    }
    size_t len = req.headers.count("content-length") ? std::strtoull(req.headers["content-length"].c_str(), nullptr, 10) : 0;
    if (len > kMaxBody) { reply(s, 413, R"({"error":"Payload too large"})"); CLOSESOCK(s); return; }
    req.rawBody = buf.substr(he + 4);
    while (req.rawBody.size() < len) {
      int n = static_cast<int>(::recv(s, tmp, sizeof tmp, 0));
      if (n <= 0) { CLOSESOCK(s); return; }
      req.rawBody.append(tmp, static_cast<size_t>(n));
    }
    Response res;
    if (req.method == "OPTIONS") { res.status(204).payload = ""; }
    else {
      std::lock_guard<std::mutex> lock(mu_);
      try { router_.dispatch(req, res); }
      catch (const std::exception& e) { std::cerr << "Error: " << e.what() << "\n"; res.status(500).send({{"error", "Server error"}}); }
    }
    reply(s, res.code, res.payload, res.contentType, res.cacheControl);
    CLOSESOCK(s);
  }

 public:
  HttpServer(const Router& r, int port) : router_(r), port_(port) {}

  void run() {
#ifdef _WIN32
    WSADATA wsa; WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
    sock_t srv = ::socket(AF_INET, SOCK_STREAM, 0);
    int yes = 1;
    ::setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&yes), sizeof yes);
    sockaddr_in addr{}; addr.sin_family = AF_INET; addr.sin_addr.s_addr = htonl(INADDR_ANY); addr.sin_port = htons(static_cast<uint16_t>(port_));
    if (::bind(srv, reinterpret_cast<sockaddr*>(&addr), sizeof addr) != 0 || ::listen(srv, 64) != 0)
      throw std::runtime_error("Cannot listen on port " + std::to_string(port_) + " (already in use?)");
    std::cout << "API running on http://localhost:" << port_ << std::endl;
    for (;;) {
      sockaddr_in ca{}; 
#ifdef _WIN32
      int cl = sizeof ca;
#else
      socklen_t cl = sizeof ca;
#endif
      sock_t c = ::accept(srv, reinterpret_cast<sockaddr*>(&ca), &cl);
      if (c == BAD_SOCK) continue;
      char ipb[64] = {0}; inet_ntop(AF_INET, &ca.sin_addr, ipb, sizeof ipb);
      std::thread(&HttpServer::serve, this, c, std::string(ipb)).detach();
    }
  }
};
