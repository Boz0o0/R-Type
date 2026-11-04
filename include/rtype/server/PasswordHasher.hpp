#pragma once

#include <array>
#include <cstdint>
#include <string_view>

/**
 * @file PasswordHasher.hpp
 * @brief Hashing of the account passwords with Argon2id.
 *
 * Passwords are never stored: only a random salt and the Argon2id hash of
 * the password with that salt.
 */

namespace rtype::server {

/** @brief Random bytes mixed into one password hash, unique per account. */
using Salt = std::array<std::uint8_t, 16>;
/** @brief Argon2id output stored for one account. */
using Hash = std::array<std::uint8_t, 32>;

/**
 * @brief Draws a salt from the random source of the system.
 *
 * @return A new salt.
 */
[[nodiscard]] Salt randomSalt();

/**
 * @brief Hashes a password with Argon2id. Slow on purpose (tens of ms).
 *
 * @param password Password given by the player.
 * @param salt Salt of the account.
 * @return The hash to store or to compare.
 */
[[nodiscard]] Hash hashPassword(std::string_view password, const Salt& salt);

/**
 * @brief Checks a password against a stored hash, in constant time.
 *
 * @param password Password given by the player.
 * @param salt Salt of the account.
 * @param expected Hash stored for the account.
 * @return true if the password is the one of the account.
 */
[[nodiscard]] bool verifyPassword(std::string_view password, const Salt& salt,
                                  const Hash& expected);

}  // namespace rtype::server
