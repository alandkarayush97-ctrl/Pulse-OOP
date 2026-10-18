#pragma once
#include <ctime>
#include <cstdio>
#include <string>
#include <vector>
#include "../core/Database.hpp"
#include "../core/Media.hpp"

inline const std::vector<std::string> ENERGY_TAGS = {"Venting", "Celebrating", "Asking advice", "Just sharing"};

// Domain model: encapsulates the visibility rules (Time Capsule + Co-Authored posts).
class Tweet {
  long long id_, authorId_;
  std::string content_, vibe_, energy_, createdAt_, authorName_, authorHandle_;
  std::string imagePrefix_; long long imageLen_ = 0;
  Json publishAt_, replyTo_, coAuthors_, reactions_, myReaction_;
  long long likes_ = 0, retweets_ = 0, replies_ = 0;
  bool likedByMe_ = false;

  static bool parseTime(const std::string& s, std::time_t& out) {
    int Y, M, D, h = 0, m = 0, sec = 0;
    if (std::sscanf(s.c_str(), "%d-%d-%d%*c%d:%d:%d", &Y, &M, &D, &h, &m, &sec) < 5) return false;
    std::tm t{}; t.tm_year = Y - 1900; t.tm_mon = M - 1; t.tm_mday = D; t.tm_hour = h; t.tm_min = m; t.tm_sec = sec; t.tm_isdst = -1;
    bool utc = !s.empty() && s.back() == 'Z';
#ifdef _WIN32
    out = utc ? _mkgmtime(&t) : std::mktime(&t);
#else
    out = utc ? timegm(&t) : std::mktime(&t);
#endif
    return out != static_cast<std::time_t>(-1);
  }

 public:
  Tweet(const Json& row, long long likes, long long retweets, long long replies, bool likedByMe,
        Json reactions, Json myReaction, Json coAuthors)
      : id_(row["id"]), authorId_(row["author_id"]), content_(row["content"]), vibe_(row.value("vibe", "")),
        energy_(row.value("energy", "")), createdAt_(row.value("created_at", "")),
        authorName_(row["author_name"]), authorHandle_(row["author_handle"]),
        publishAt_(row["publish_at"]), replyTo_(row["reply_to"]),
        coAuthors_(std::move(coAuthors)), reactions_(std::move(reactions)), myReaction_(std::move(myReaction)),
        likes_(likes), retweets_(retweets), replies_(replies), likedByMe_(likedByMe) {
    if (row["image_url"].is_string()) { imagePrefix_ = row["image_url"].get<std::string>(); imageLen_ = row["image_len"].is_number() ? row["image_len"].get<long long>() : 1; }
  }

  long long id() const { return id_; }
  long long authorId() const { return authorId_; }
  const std::string& content() const { return content_; }

  bool isLocked() const {
    std::time_t t;
    return publishAt_.is_string() && !publishAt_.get<std::string>().empty() && parseTime(publishAt_.get<std::string>(), t) && t > std::time(nullptr);
  }
  bool hasPendingCoauthors() const {
    for (const auto& c : coAuthors_) if (c["status"] == "pending") return true;
    return false;
  }
  bool isParticipant(long long userId) const {
    if (authorId_ == userId) return true;
    for (const auto& c : coAuthors_) if (c["id"].get<long long>() == userId) return true;
    return false;
  }
  bool isVisibleTo(long long userId) const {
    if (isLocked() && authorId_ != userId) return false;
    if (hasPendingCoauthors() && !isParticipant(userId)) return false;
    return true;
  }

  Json toJson() const {
    Json imageUrl = nullptr, mediaType = nullptr;
    if (!imagePrefix_.empty()) {
      imageUrl = Media::base() + "/api/media/tweets/" + std::to_string(id_) + "?v=" + std::to_string(imageLen_);
      mediaType = Media::typeOf(imagePrefix_);
    }
    return {{"id", id_}, {"content", content_}, {"vibe", vibe_}, {"energy", energy_}, {"publishAt", publishAt_},
            {"locked", isLocked()}, {"replyTo", replyTo_}, {"imageUrl", imageUrl}, {"mediaType", mediaType}, {"createdAt", createdAt_},
            {"author", {{"id", authorId_}, {"name", authorName_}, {"handle", authorHandle_}}},
            {"likes", likes_}, {"retweets", retweets_}, {"replies", replies_}, {"likedByMe", likedByMe_},
            {"reactions", reactions_}, {"myReaction", myReaction_}, {"coAuthors", coAuthors_},
            {"pendingCoauthor", hasPendingCoauthors()}};
  }
};
