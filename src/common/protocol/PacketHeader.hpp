#pragma once

#include <cstdint>

#include "common/protocol/Protocol.hpp"

namespace rtype::protocol {

/**
 * @brief The 12-byte header that starts every datagram.
 *
 * This struct is never copied to the network as-is: the codec writes each
 * field one by one in big-endian order, so padding and the host endianness
 * do not matter.
 *
 * @note The header is always 12 bytes long, so the payload starts at offset 12.
 */
struct PacketHeader {
  /** @brief Always MAGIC. */
  std::uint16_t magic{MAGIC};
  /** @brief Protocol version of the sender. */
  std::uint8_t version{VERSION};
  /** @brief Type of the payload that follows. */
  PacketType type{};
  /** @brief PacketFlag bits. */
  std::uint8_t flags{};
  /** @brief Sent as 0, ignored on receive. */
  std::uint8_t reserved{};
  /** @brief Number of bytes after the header. */
  std::uint16_t payloadSize{};
  /** @brief Per-sender counter, incremented for each packet sent. */
  std::uint32_t sequence{};

  /** @brief Whether the receiver must answer with an ACK. */
  [[nodiscard]] bool isReliable() const noexcept {
    return (this->flags & PacketFlag::RELIABLE) != 0;
  }
};

}  // namespace rtype::protocol
