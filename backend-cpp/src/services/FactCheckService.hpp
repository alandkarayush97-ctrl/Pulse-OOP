#pragma once
#include <algorithm>
#include <cctype>
#include <set>
#include <string>
#include <vector>
#include "../core/Database.hpp"

// Context Check: multi-signal heuristic neutrality scorer (no external API).
// Each signal adds or subtracts points, so different posts get different scores.
class FactCheckService {
  const std::set<std::string> charged_ = {"disaster","evil","stupid","insane","outrageous","destroy","ruin","worst","unbelievable","shocking",
    "terrifying","hate","disgusting","idiot","idiots","pathetic","garbage","trash","scam","lies","liar","liars","corrupt","horrible","awful",
    "terrible","ridiculous","crazy","nonsense","fraud","traitor","scum","furious","outrage","betrayal","hell","damn","sucks","moron","clown","joke"};
  const std::set<std::string> absolutes_ = {"always","never","everyone","everybody","nobody","everything","nothing","totally","completely",
    "absolutely","literally","obviously","undeniable","guaranteed","proven","definitely","entire","forever","impossible"};
  const std::vector<std::string> opinion_ = {"i think","i feel","i believe","in my opinion","imo","imho","i guess","personally"};
  const std::vector<std::string> evidence_ = {"according to","study","studies","research","report","reported","data","survey","source","statistics","percent","official","published","evidence"};
  const std::set<std::string> hedges_ = {"may","might","could","possibly","perhaps","suggests","appears","likely","probably","reportedly","allegedly"};

  static int countPhrase(const std::string& text, const std::string& p) {
    int n = 0;
    for (size_t pos = text.find(p); pos != std::string::npos; pos = text.find(p, pos + p.size())) {
      bool l = pos == 0 || !std::isalnum((unsigned char)text[pos - 1]);
      size_t e = pos + p.size();
      bool r = e >= text.size() || !std::isalnum((unsigned char)text[e]);
      if (l && r) ++n;
    }
    return n;
  }
  static void add(Json& signals, const std::string& label, const std::string& detail, int impact) {
    signals.push_back({{"label", label}, {"detail", detail}, {"impact", impact}});
  }
 public:
  Json analyze(const std::string& content) const {
    std::vector<std::string> words, rawWords; std::string cur, raw;
    for (unsigned char c : content) {
      if (std::isalnum(c) || c == '\'') { raw += static_cast<char>(c); cur += static_cast<char>(std::tolower(c)); }
      else if (!cur.empty()) { words.push_back(cur); rawWords.push_back(raw); cur.clear(); raw.clear(); }
    }
    if (!cur.empty()) { words.push_back(cur); rawWords.push_back(raw); }
    std::string lower = content; for (auto& c : lower) c = static_cast<char>(std::tolower((unsigned char)c));

    Json flagged = Json::array(), signals = Json::array();
    int score = 72, chargedN = 0, absN = 0, capsN = 0, hedgeN = 0;
    for (size_t i = 0; i < words.size(); ++i) {
      if (charged_.count(words[i])) { ++chargedN; flagged.push_back(words[i]); }
      else if (absolutes_.count(words[i])) { ++absN; flagged.push_back(words[i]); }
      if (hedges_.count(words[i])) ++hedgeN;
      bool allCaps = rawWords[i].size() >= 3 && std::all_of(rawWords[i].begin(), rawWords[i].end(), [](unsigned char c) { return !std::isalpha(c) || std::isupper(c); })
                     && std::any_of(rawWords[i].begin(), rawWords[i].end(), [](unsigned char c) { return std::isalpha(c); });
      if (allCaps) { ++capsN; flagged.push_back(rawWords[i]); }
    }
    int excl = (int)std::count(content.begin(), content.end(), '!');
    int opinionN = 0, evidenceN = 0;
    for (const auto& p : opinion_) opinionN += countPhrase(lower, p);
    for (const auto& p : evidence_) evidenceN += countPhrase(lower, p);
    bool hasDigit = std::any_of(content.begin(), content.end(), [](unsigned char c) { return std::isdigit(c); });
    bool hasLink = lower.find("http://") != std::string::npos || lower.find("https://") != std::string::npos || lower.find("www.") != std::string::npos;

    auto apply = [&](int n, int each, int cap, const std::string& label, const std::string& unit) {
      if (!n) return;
      int impact = std::max(-cap, std::min(cap, n * each));
      score += impact; add(signals, label, std::to_string(n) + " " + unit, impact);
    };
    apply(chargedN, -9, 45, "Charged language", "emotionally charged word(s)");
    apply(absN, -6, 24, "Absolute claims", "absolute word(s) like always/never");
    apply(capsN, -5, 20, "SHOUTING", "ALL-CAPS word(s)");
    apply(excl, -3, 15, "Exclamation marks", "exclamation mark(s)");
    apply(opinionN, -4, 12, "Opinion framing", "opinion phrase(s)");
    apply(evidenceN, 7, 21, "Evidence cues", "source/data cue(s)");
    apply(hedgeN, 3, 9, "Careful wording", "hedging word(s)");
    if (hasDigit) { score += 5; add(signals, "Specific figures", "contains numbers", 5); }
    if (hasLink) { score += 8; add(signals, "Link included", "points to a source", 8); }
    if (words.size() >= 12) { score += 3; add(signals, "Context", "longer post gives more context", 3); }
    if (words.size() < 4) { score -= 10; add(signals, "Very short", "too little text to judge", -10); }

    score = std::max(5, std::min(98, score));
    std::string summary =
      score >= 70 ? "This post reads as largely neutral and fact-oriented, with little emotionally charged language."
      : score >= 40 ? "This post contains some opinionated or emotionally charged language. Consider cross-checking claims."
      : "This post uses strong, absolute, or emotionally charged language. Treat claims with extra scrutiny.";
    return {{"neutralityScore", score}, {"summary", summary}, {"flaggedWords", flagged}, {"signals", signals},
            {"note", "Automated heuristic analysis, not a verified fact-check. No external sources consulted."}};
  }
};
