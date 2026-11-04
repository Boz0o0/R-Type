#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>

/**
 * @file LoginLimiter.hpp
 * @brief Limit of the login attempts of each IP address.
 */

namespace rtype::server {

/**
 * @brief Counts the LOGIN and REGISTER of each IP address, to stop password
 *        guessing and to bound the Argon2 work one address can ask for.
 *
 * An address gets MAX_ATTEMPTS attempts per WINDOW. An attempt counts as soon
 * as its password is sent to be hashed, and is given back if it succeeds: so
 * only the failed attempts, and the ones in progress, count. Keyed by IP
 * address, not by port, since a client can pick any port.
 */
class LoginLimiter {
 public:
  using Clock = std::chrono::steady_clock;

  /** @brief Failed (or running) attempts allowed per WINDOW. */
  static constexpr std::uint32_t MAX_ATTEMPTS = 5;
  /** @brief Length of a counting window. */
  static constexpr std::chrono::seconds WINDOW{60};
  /**
   * @brief Addresses tracked at once. Past it, a new address is refused
   *        until a window ends: memory stays bounded under a flood of
   *        fake addresses.
   */
  static constexpr std::size_t MAX_ADDRESSES = 4096;

  /**
   * @brief Counts an attempt, if the address has some left.
   *
   * @param address IP address of the sender.
   * @param now Current time.
   * @return false if the attempt must be refused.
   */
  [[nodiscard]] bool tryAttempt(const std::string& address,
                                Clock::time_point now);

  /**
   * @brief Gives an attempt back, after a successful login.
   *
   * @param address IP address of the sender.
   */
  void refund(const std::string& address);

 private:
  struct Entry {
    /** @brief Start of the current window of this address. */
    Clock::time_point windowStart;
    /** @brief Attempts counted in it. */
    std::uint32_t attempts{};
  };

  /** @brief Forgets the addresses whose window is over. */
  void forgetExpired(Clock::time_point now);

  std::unordered_map<std::string, Entry> _entries;
};

}  // namespace rtype::server
