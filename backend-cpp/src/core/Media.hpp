#pragma once
#include <string>
// Public base URL used to build media links (set once in main).
struct Media {
  static std::string& base() { static std::string b = "http://localhost:4000"; return b; }
  // "data:image/png;base64,...." prefix -> "image"
  static std::string typeOf(const std::string& prefix) {
    if (prefix.rfind("data:", 0) != 0) return "";
    size_t slash = prefix.find('/'); return slash == std::string::npos ? "" : prefix.substr(5, slash - 5);
  }
};
