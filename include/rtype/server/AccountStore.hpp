#pragma once

#include <cstdint>
#include <optional>
#include <string>

/**
 * @file AccountStore.hpp
 * @brief Player accounts, kept in a SQLite database.
 */

struct sqlite3;

namespace rtype::server {

/**
 * @brief The player accounts, stored in a SQLite file.
 *
 * Each account keeps its username, a random salt and the Argon2id hash of
 * its password: never the password itself. Not thread-safe: only the Lobby
 * thread uses it.
 */
class AccountStore {
 public:
  /** @brief Id of an account in the database. */
  using AccountId = std::int64_t;

  /**
   * @brief Opens the database, and creates its table if it is new.
   *
   * @param path File of the database, or ":memory:" for one in memory.
   * @throws std::runtime_error if the database cannot be opened.
   */
  explicit AccountStore(const std::string& path);

  /** @brief Closes the database. */
  ~AccountStore();

  AccountStore(const AccountStore&) = delete;
  AccountStore& operator=(const AccountStore&) = delete;
  AccountStore(AccountStore&&) = delete;
  AccountStore& operator=(AccountStore&&) = delete;

  /**
   * @brief Creates an account.
   *
   * @param username Name of the account, unique ignoring case.
   * @param password Password of the account, hashed before being stored.
   * @return The id of the new account, or nothing if the name is taken.
   */
  [[nodiscard]] std::optional<AccountId> create(const std::string& username,
                                                const std::string& password);

  /**
   * @brief Checks a username and a password.
   *
   * Takes about as long for an unknown username as for a wrong password, so
   * the answer time does not tell which usernames exist.
   *
   * @param username Name of the account, any case.
   * @param password Password given by the player.
   * @return The id of the account, or nothing if they do not match.
   */
  [[nodiscard]] std::optional<AccountId> authenticate(
      const std::string& username, const std::string& password);

 private:
  sqlite3* _db{};
};

}  // namespace rtype::server
