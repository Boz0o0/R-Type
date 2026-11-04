#include <rtype/server/AccountStore.hpp>
#include <memory>
#include <rtype/server/PasswordHasher.hpp>
#include <sqlite3.h>
#include <stdexcept>

using Statement = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;

Statement prepare(sqlite3* db, const char* sql) {
  sqlite3_stmt* raw = nullptr;
  if (sqlite3_prepare_v2(db, sql, -1, &raw, nullptr) != SQLITE_OK)
    throw std::runtime_error(sqlite3_errmsg(db));
  return {raw, &sqlite3_finalize};

}

namespace rtype::server {

AccountStore::AccountStore(const std::string& path) {
    if (sqlite3_open(path.c_str(), &(this->_db)) != SQLITE_OK)
        throw std::runtime_error("cannot open database " + path);
    sqlite3_exec(this->_db,
        "CREATE TABLE IF NOT EXISTS accounts ("
        "  id INTEGER PRIMARY KEY,"
        "  username TEXT UNIQUE NOT NULL COLLATE NOCASE,"
        "  pwd_salt BLOB NOT NULL,"
        "  pwd_hash BLOB NOT NULL);",
        nullptr, nullptr, nullptr);
}

AccountStore::~AccountStore() { sqlite3_close(_db); }

bool AccountStore::create(const std::string& username, const std::string& password) {
    const Salt salt = randomSalt();
    const Hash hash = hashPassword(password, salt);
    auto stmt = prepare(this->_db,
        "INSERT INTO accounts (username, pwd_salt, pwd_hash) VALUES (?, ?, ?);");
    sqlite3_bind_text(stmt.get(), 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt.get(), 2, salt.data(), salt.size(), SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt.get(), 3, hash.data(), hash.size(), SQLITE_TRANSIENT);
    return sqlite3_step(stmt.get()) == SQLITE_DONE;
}

std::optional<std::int64_t> AccountStore::authenticate(const std::string& username,
                                                       const std::string& password) {
    auto stmt = prepare(_db,
        "SELECT id, pwd_salt, pwd_hash FROM accounts WHERE username = ?;");

    sqlite3_bind_text(stmt.get(), 1, username.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt.get()) != SQLITE_ROW)
        return std::nullopt;
    Salt salt{};
    Hash hash{};
    std::memcpy(salt.data(), sqlite3_column_blob(stmt.get(), 1), salt.size());
    std::memcpy(hash.data(), sqlite3_column_blob(stmt.get(), 2), hash.size());
    if (!verifyPassword(password, salt, hash)) return std::nullopt;
    return sqlite3_column_int64(stmt.get(), 0);
}

}