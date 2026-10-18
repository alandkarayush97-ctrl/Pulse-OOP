#pragma once
#include <optional>
#include <string>
#include "../core/Database.hpp"

// Abstract base repository (pure virtual entityName() makes it non-instantiable).
class BaseRepository {
 protected:
  Database& db_;
  std::string table_;
  BaseRepository(Database& db, std::string table) : db_(db), table_(std::move(table)) {}
 public:
  virtual ~BaseRepository() = default;
  virtual std::string entityName() const = 0;

  std::optional<Json> findById(long long id) { return db_.one("SELECT * FROM " + table_ + " WHERE id = ?", {id}); }
  virtual void remove(long long id) { db_.exec("DELETE FROM " + table_ + " WHERE id = ?", {id}); }  // overridable
  long long count(const std::string& where = "1", const std::vector<Json>& p = {}) {
    return db_.scalar("SELECT COUNT(*) c FROM " + table_ + " WHERE " + where, p);
  }
};
