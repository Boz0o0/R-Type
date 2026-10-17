#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <rtype/common/protocol/Protocol.hpp>
#include <span>
#include <vector>

namespace rtype::server {

/**
 * @brief The reliable packets sent to one client and not acknowledged yet.
 *
 * Each one is sent again, with the same bytes, every RESEND_INTERVAL until
 * its ACK arrives. After MAX_SEND_ATTEMPTS sends without ACK, the client is
 * considered lost.
 */
class ReliableOutbox {
 public:
  using Clock = std::chrono::steady_clock;
  /** @brief Function that puts a datagram on the network. */
  using SendFunction = std::function<void(std::span<const std::byte>)>;

  /** @brief Delay before a packet without ACK is sent again. */
  static constexpr std::chrono::milliseconds RESEND_INTERVAL{
      protocol::RESEND_INTERVAL_MS};

  /**
   * @brief Keeps a reliable packet that was just sent for the first time.
   *
   * @param sequence Sequence written in its header.
   * @param datagram Its bytes, header included.
   * @param now When it was sent.
   */
  void add(std::uint32_t sequence, std::vector<std::byte> datagram,
           Clock::time_point now);

  /** @brief Forgets the packet of an ACK; unknown sequences are ignored. */
  void acknowledge(std::uint32_t sequence);

  /**
   * @brief Sends again every packet whose last send is RESEND_INTERVAL old.
   *
   * @param now Current time.
   * @param send Called with the bytes of each packet to send again.
   * @return false if a packet reached MAX_SEND_ATTEMPTS without ACK: the
   *         client is lost.
   */
  [[nodiscard]] bool resendDue(Clock::time_point now, const SendFunction& send);

  /** @brief Number of packets waiting for their ACK. */
  [[nodiscard]] std::size_t size() const noexcept;

 private:
  struct Pending {
    std::uint32_t sequence{};
    std::vector<std::byte> datagram;
    Clock::time_point lastSent;
    std::uint32_t attempts{};
  };

  std::vector<Pending> _pending;
};

}  // namespace rtype::server
