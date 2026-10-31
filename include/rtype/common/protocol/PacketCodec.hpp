#pragma once

#include <cstddef>
#include <cstdint>
#include <rtype/common/protocol/PacketHeader.hpp>
#include <rtype/common/protocol/Packets.hpp>
#include <span>
#include <string_view>
#include <variant>
#include <vector>

namespace rtype::protocol {

/** @brief Any decoded payload. */
using Payload =
    std::variant<ConnectRequest, ConnectAccept, ConnectReject, Disconnect, Ping,
                 Pong, Ack, PlayerInput, GameStart, PlayerJoined, PlayerLeft,
                 EntitySpawn, EntityDestroy, WorldSnapshot, PlayerState,
                 GameOver, RoomList, RoomCreate, RoomJoin, RoomLeave,
                 RoomListReply, RoomJoined, RoomReject, RoomLeft>;

/** @brief A decoded datagram. */
struct Packet {
  PacketHeader header;
  Payload payload;
};

/** @brief Why decode() refused a datagram. */
enum class DecodeError : std::uint8_t {
  /** @brief The datagram is valid. */
  None,
  /** @brief Shorter than HEADER_SIZE. */
  TooShort,
  /** @brief Longer than MAX_DATAGRAM_SIZE. */
  TooLarge,
  /** @brief The first two bytes are not MAGIC. */
  BadMagic,
  /** @brief The version is not VERSION; Packet::header is still filled. */
  VersionMismatch,
  /** @brief payloadSize does not match the datagram size. */
  SizeMismatch,
  /** @brief The type is not a PacketType. */
  UnknownType,
  /** @brief The payload is too short, too long or holds invalid values. */
  MalformedPayload,
};

/**
 * @brief The PacketType of a payload.
 *
 * @param payload Payload to inspect.
 * @return The TYPE of the struct it holds.
 */
[[nodiscard]] PacketType typeOf(const Payload& payload);

/**
 * @brief Turns a payload into a datagram ready to send.
 *
 * The RELIABLE flag is set from isReliable().
 *
 * @param payload What to send.
 * @param sequence Next sequence number of the sender.
 * @return The bytes of the datagram, header included.
 * @throws ProtocolError If the payload breaks a limit of the protocol
 *         (name too long, too many entities in a snapshot...).
 */
[[nodiscard]] std::vector<std::byte> encode(const Payload& payload,
                                            std::uint32_t sequence);

/**
 * @brief Checks and decodes a received datagram.
 *
 * Never reads out of bounds, whatever the content of the datagram.
 * The receive buffer should be at least MAX_DATAGRAM_SIZE + 1 bytes, so
 * that a datagram too large is reported as TooLarge instead of being cut.
 *
 * @param datagram Bytes received.
 * @param out Filled when the result is None.
 * @return DecodeError::None on success, the first problem found otherwise.
 */
[[nodiscard]] DecodeError decode(std::span<const std::byte> datagram,
                                 Packet& out);

/** @brief Human-readable name of a DecodeError, for logs. */
[[nodiscard]] std::string_view toString(DecodeError error) noexcept;

}  // namespace rtype::protocol
