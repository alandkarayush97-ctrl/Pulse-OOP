#pragma once
// Crypto primitives + password hashing + JWT. Self-contained (no OpenSSL needed).
#include <cstdint>
#include <ctime>
#include <random>
#include <string>
#include <vector>
#include "nlohmann/json.hpp"

namespace crypto {
using Bytes = std::vector<uint8_t>;

inline Bytes toBytes(const std::string& s) { return Bytes(s.begin(), s.end()); }

inline Bytes sha256(const Bytes& data) {
  static const uint32_t K[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
  uint32_t h[8] = {0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
  Bytes m = data;
  uint64_t bits = static_cast<uint64_t>(data.size()) * 8;
  m.push_back(0x80);
  while (m.size() % 64 != 56) m.push_back(0);
  for (int i = 7; i >= 0; --i) m.push_back(static_cast<uint8_t>(bits >> (i * 8)));
  auto rotr = [](uint32_t x, int n) { return (x >> n) | (x << (32 - n)); };
  for (size_t off = 0; off < m.size(); off += 64) {
    uint32_t w[64];
    for (int i = 0; i < 16; ++i)
      w[i] = (uint32_t(m[off+i*4])<<24)|(uint32_t(m[off+i*4+1])<<16)|(uint32_t(m[off+i*4+2])<<8)|uint32_t(m[off+i*4+3]);
    for (int i = 16; i < 64; ++i) {
      uint32_t s0 = rotr(w[i-15],7)^rotr(w[i-15],18)^(w[i-15]>>3);
      uint32_t s1 = rotr(w[i-2],17)^rotr(w[i-2],19)^(w[i-2]>>10);
      w[i] = w[i-16] + s0 + w[i-7] + s1;
    }
    uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],hh=h[7];
    for (int i = 0; i < 64; ++i) {
      uint32_t S1 = rotr(e,6)^rotr(e,11)^rotr(e,25), ch = (e&f)^(~e&g);
      uint32_t t1 = hh + S1 + ch + K[i] + w[i];
      uint32_t S0 = rotr(a,2)^rotr(a,13)^rotr(a,22), mj = (a&b)^(a&c)^(b&c);
      uint32_t t2 = S0 + mj;
      hh=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
    }
    h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=hh;
  }
  Bytes out;
  for (int i = 0; i < 8; ++i) for (int j = 3; j >= 0; --j) out.push_back(static_cast<uint8_t>(h[i] >> (j * 8)));
  return out;
}

inline Bytes hmacSha256(Bytes key, const Bytes& msg) {
  if (key.size() > 64) key = sha256(key);
  key.resize(64, 0);
  Bytes inner(64), outer(64);
  for (int i = 0; i < 64; ++i) { inner[i] = key[i] ^ 0x36; outer[i] = key[i] ^ 0x5c; }
  inner.insert(inner.end(), msg.begin(), msg.end());
  Bytes ih = sha256(inner);
  outer.insert(outer.end(), ih.begin(), ih.end());
  return sha256(outer);
}

inline Bytes pbkdf2(const Bytes& pw, const Bytes& salt, int iterations) {
  Bytes s = salt; s.push_back(0); s.push_back(0); s.push_back(0); s.push_back(1);
  Bytes u = hmacSha256(pw, s), t = u;
  for (int i = 1; i < iterations; ++i) {
    u = hmacSha256(pw, u);
    for (size_t j = 0; j < t.size(); ++j) t[j] ^= u[j];
  }
  return t;
}

inline std::string hex(const Bytes& b) {
  static const char* d = "0123456789abcdef";
  std::string s;
  for (uint8_t c : b) { s += d[c >> 4]; s += d[c & 15]; }
  return s;
}
inline Bytes unhex(const std::string& s) {
  Bytes b;
  for (size_t i = 0; i + 1 < s.size(); i += 2) b.push_back(static_cast<uint8_t>(std::stoi(s.substr(i, 2), nullptr, 16)));
  return b;
}
inline bool constantTimeEq(const Bytes& a, const Bytes& b) {
  if (a.size() != b.size()) return false;
  uint8_t r = 0;
  for (size_t i = 0; i < a.size(); ++i) r |= a[i] ^ b[i];
  return r == 0;
}

inline std::string b64url(const Bytes& in) {
  static const char* t = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
  std::string out; size_t i = 0;
  for (; i + 2 < in.size(); i += 3) {
    uint32_t v = (in[i] << 16) | (in[i+1] << 8) | in[i+2];
    out += t[v>>18]; out += t[(v>>12)&63]; out += t[(v>>6)&63]; out += t[v&63];
  }
  if (i + 1 == in.size()) { uint32_t v = in[i] << 16; out += t[v>>18]; out += t[(v>>12)&63]; }
  else if (i + 2 == in.size()) { uint32_t v = (in[i] << 16) | (in[i+1] << 8); out += t[v>>18]; out += t[(v>>12)&63]; out += t[(v>>6)&63]; }
  return out;
}
inline Bytes unb64url(const std::string& s) {
  Bytes out; uint32_t buf = 0; int bits = 0;
  for (char c : s) {
    int v;
    if (c >= 'A' && c <= 'Z') v = c - 'A';
    else if (c >= 'a' && c <= 'z') v = c - 'a' + 26;
    else if (c >= '0' && c <= '9') v = c - '0' + 52;
    else if (c == '-') v = 62; else if (c == '_') v = 63; else return {};
    buf = (buf << 6) | v; bits += 6;
    if (bits >= 8) { bits -= 8; out.push_back(static_cast<uint8_t>((buf >> bits) & 0xFF)); }
  }
  return out;
}
inline std::string b64decode(const std::string& in) {
  std::string out; uint32_t buf = 0; int bits = 0;
  for (unsigned char c : in) {
    int v;
    if (c >= 'A' && c <= 'Z') v = c - 'A'; else if (c >= 'a' && c <= 'z') v = c - 'a' + 26;
    else if (c >= '0' && c <= '9') v = c - '0' + 52; else if (c == '+') v = 62; else if (c == '/') v = 63; else continue;
    buf = (buf << 6) | v; bits += 6;
    if (bits >= 8) { bits -= 8; out += static_cast<char>((buf >> bits) & 0xFF); }
  }
  return out;
}
}  // namespace crypto

// ---- Password hashing: interface + PBKDF2 implementation (polymorphism / DI) ----
class IPasswordHasher {
 public:
  virtual ~IPasswordHasher() = default;
  virtual std::string hash(const std::string& password) const = 0;
  virtual bool verify(const std::string& password, const std::string& stored) const = 0;
};

class Pbkdf2Hasher final : public IPasswordHasher {
  static constexpr int kIterations = 20000;
 public:
  std::string hash(const std::string& password) const override {
    std::random_device rd;
    crypto::Bytes salt(16);
    for (auto& b : salt) b = static_cast<uint8_t>(rd());
    auto dk = crypto::pbkdf2(crypto::toBytes(password), salt, kIterations);
    return "pbkdf2$" + std::to_string(kIterations) + "$" + crypto::hex(salt) + "$" + crypto::hex(dk);
  }
  bool verify(const std::string& password, const std::string& stored) const override {
    size_t p1 = stored.find('$'), p2 = stored.find('$', p1 + 1), p3 = stored.find('$', p2 + 1);
    if (p1 == std::string::npos || p2 == std::string::npos || p3 == std::string::npos) return false;
    int iters = std::atoi(stored.substr(p1 + 1, p2 - p1 - 1).c_str());
    auto salt = crypto::unhex(stored.substr(p2 + 1, p3 - p2 - 1));
    auto want = crypto::unhex(stored.substr(p3 + 1));
    return crypto::constantTimeEq(crypto::pbkdf2(crypto::toBytes(password), salt, iters), want);
  }
};

// ---- JWT (HS256) ----
class JwtService {
  std::string secret_;
 public:
  explicit JwtService(std::string secret) : secret_(std::move(secret)) {}

  std::string sign(long long userId, long long ttlSeconds = 7 * 24 * 3600) const {
    nlohmann::json payload = {{"id", userId}, {"exp", static_cast<long long>(std::time(nullptr)) + ttlSeconds}};
    std::string head = crypto::b64url(crypto::toBytes(R"({"alg":"HS256","typ":"JWT"})"));
    std::string body = crypto::b64url(crypto::toBytes(payload.dump()));
    std::string data = head + "." + body;
    return data + "." + crypto::b64url(crypto::hmacSha256(crypto::toBytes(secret_), crypto::toBytes(data)));
  }

  // Returns user id, or -1 if invalid/expired.
  long long verify(const std::string& token) const {
    size_t d1 = token.find('.'), d2 = token.rfind('.');
    if (d1 == std::string::npos || d1 == d2) return -1;
    std::string data = token.substr(0, d2);
    auto want = crypto::hmacSha256(crypto::toBytes(secret_), crypto::toBytes(data));
    if (!crypto::constantTimeEq(want, crypto::unb64url(token.substr(d2 + 1)))) return -1;
    auto raw = crypto::unb64url(token.substr(d1 + 1, d2 - d1 - 1));
    auto j = nlohmann::json::parse(std::string(raw.begin(), raw.end()), nullptr, false);
    if (!j.is_object() || !j.contains("id") || !j.contains("exp")) return -1;
    if (j["exp"].get<long long>() < static_cast<long long>(std::time(nullptr))) return -1;
    return j["id"].get<long long>();
  }
};
