#include <sqlite3.h>

#include <cstring>
#include <memory>
#include <optional>
#include <rtype/server/AccountStore.hpp>
#include <rtype/server/PasswordHasher.hpp>
#include <stdexcept>
#include <string>

namespace rtype::server {

namespace {

/** @brief A prepared statement, finalized when it goes out of scope. */
using Statement = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;

Statement prepare(sqlite3* database, const char* sql) {
  sqlite3_stmt* raw = nullptr;
  if (sqlite3_prepare_v2(database, sql, -1, &raw, nullptr) != SQLITE_OK) {
    throw std::runtime_error(std::string("sqlite: ") +
                             sqlite3_errmsg(database));
  }
  return {raw, &sqlite3_finalize};
}

/** @brief Copies a BLOB column into an array, if it has the right size. */
template <typename Bytes>
bool readBlob(sqlite3_stmt* statement, int column, Bytes& out) {
  const void* data = sqlite3_column_blob(statement, column);
  if (data == nullptr ||
      sqlite3_column_bytes(statement, column) != static_cast<int>(out.size())) {
    return false;
  }
  std::memcpy(out.data(), data, out.size());
  return true;
}

}  // namespace

AccountStore::AccountStore(const std::string& path) {
  if (sqlite3_open(path.c_str(), &this->_db) != SQLITE_OK) {
    const std::string error = sqlite3_errmsg(this->_db);
    sqlite3_close(this->_db);
    throw std::runtime_error("cannot open database " + path + ": " + error);
  }
  char* error = nullptr;
  if (sqlite3_exec(this->_db,
                   "CREATE TABLE IF NOT EXISTS accounts ("
                   "  id INTEGER PRIMARY KEY,"
                   "  username TEXT UNIQUE NOT NULL COLLATE NOCASE,"
                   "  pwd_salt BLOB NOT NULL,"
                   "  pwd_hash BLOB NOT NULL);",
                   nullptr, nullptr, &error) != SQLITE_OK) {
    const std::string message = error != nullptr ? error : "unknown error";
    sqlite3_free(error);
    sqlite3_close(this->_db);
    throw std::runtime_error("cannot create the accounts table: " + message);
  }
}

AccountStore::~AccountStore() { sqlite3_close(this->_db); }

std::optional<AccountStore::AccountId> AccountStore::create(
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    const std::string& username, const std::string& password) {
  const Salt salt = randomSalt();
  const Hash hash = hashPassword(password, salt);
  const Statement statement = prepare(
      this->_db,
      "INSERT INTO accounts (username, pwd_salt, pwd_hash) VALUES (?, ?, ?);");
  sqlite3_bind_text(statement.get(), 1, username.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_blob(statement.get(), 2, salt.data(),
                    static_cast<int>(salt.size()), SQLITE_TRANSIENT);
  sqlite3_bind_blob(statement.get(), 3, hash.data(),
                    static_cast<int>(hash.size()), SQLITE_TRANSIENT);
  if (sqlite3_step(statement.get()) != SQLITE_DONE) {
    return std::nullopt;  // the username is taken (UNIQUE)
  }
  return sqlite3_last_insert_rowid(this->_db);
}

std::optional<AccountStore::AccountId> AccountStore::authenticate(
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    const std::string& username, const std::string& password) {
  const Statement statement = prepare(
      this->_db,
      "SELECT id, pwd_salt, pwd_hash FROM accounts WHERE username = ?;");
  sqlite3_bind_text(statement.get(), 1, username.c_str(), -1, SQLITE_TRANSIENT);

  Salt salt{};
  Hash hash{};
  const bool found = sqlite3_step(statement.get()) == SQLITE_ROW &&
                     readBlob(statement.get(), 1, salt) &&
                     readBlob(statement.get(), 2, hash);
  // Hash even when the account is unknown, to take the same time.
  const bool matches = verifyPassword(password, salt, hash);
  if (!found || !matches) {
    return std::nullopt;
  }
  return sqlite3_column_int64(statement.get(), 0);
}

}  // namespace rtype::server
