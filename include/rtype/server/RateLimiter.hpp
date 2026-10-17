#pragma once

#include <ampersand/net/UdpSocket.hpp>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace rtype::server {

/**
 * @brief Counts the datagrams of each sender (IP + port) per second, so that
 *        one sender flooding the server cannot crowd out the others.
 *
 * The counters are reset every second. At most maxSenders senders are
 * counted in a second: the datagrams of more senders are refused until the
 * next second, so a flood of spoofed addresses cannot use all the memory.
 */
class RateLimiter {
 public:
  using Clock = std::chrono::steady_clock;

  /** @brief Length of a counting window. */
  static constexpr std::chrono::seconds WINDOW{1};

  struct Limits {
    /** @brief Datagrams accepted from one sender per second. */
    std::uint32_t packetsPerSecond{};
    /** @brief Senders counted per second. */
    std::size_t senders{};
  };

  /** @brief Creates a limiter that has counted nothing yet. */
  explicit RateLimiter(Limits limits);

  /**
   * @brief Counts a datagram.
   *
   * @param sender Who sent it.
   * @param now When it arrived.
   * @return false if it must be dropped.
   */
  [[nodiscard]] bool allow(const ampersand::net::Endpoint& sender,
                           Clock::time_point now);

 private:
  Limits _limits;
  Clock::time_point _windowStart;
  std::unordered_map<std::string, std::uint32_t> _counts;
};

}  // namespace rtype::server
