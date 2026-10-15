#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

/**
 * @file Protocol.hpp
 * @brief Constants and enumerations of the R-Type binary protocol (RTP/1).
 *
 * Every value here is part of the wire format described in
 * docs/mkdocs/protocol/specification.md: changing one of them breaks
 * compatibility with existing clients and servers, so bump VERSION.
 */

namespace rtype::protocol {

/** @brief First two bytes of every datagram ("RT" in ASCII). */
inline constexpr std::uint16_t MAGIC = 0x5254;
/** @brief Version of the protocol implemented by this code. */
inline constexpr std::uint8_t VERSION = 1;
/** @brief UDP port the server listens on when none is given. */
inline constexpr std::uint16_t DEFAULT_PORT = 4242;

/** @brief Size of the fixed header that starts every datagram. */
inline constexpr std::size_t HEADER_SIZE = 12;
/** @brief Largest datagram allowed, chosen to stay under a 1280-byte MTU. */
inline constexpr std::size_t MAX_DATAGRAM_SIZE = 1200;
/** @brief Largest payload allowed after the header. */
inline constexpr std::size_t MAX_PAYLOAD_SIZE = MAX_DATAGRAM_SIZE - HEADER_SIZE;

/** @brief Number of players in one game. Player ids go from 0 to 3. */
inline constexpr std::uint8_t MAX_PLAYERS = 4;
/** @brief Longest player name, in bytes of UTF-8. */
inline constexpr std::size_t MAX_PLAYER_NAME_LENGTH = 16;
/** @brief ownerId value of an entity that belongs to no player. */
inline constexpr std::uint8_t NO_OWNER = 0xFF;

/** @brief Simulation steps per second on the server. */
inline constexpr std::uint8_t SERVER_TICK_RATE = 60;
/** @brief WORLD_SNAPSHOT packets sent per second to each client. */
inline constexpr std::uint8_t SNAPSHOT_RATE = 30;

/** @brief Delay before a reliable packet without ACK is sent again. */
inline constexpr std::uint32_t RESEND_INTERVAL_MS = 100;
/** @brief Sends of a reliable packet before the peer is considered lost. */
inline constexpr std::uint32_t MAX_SEND_ATTEMPTS = 20;
/** @brief Delay between two PING packets on an idle connection. */
inline constexpr std::uint32_t PING_INTERVAL_MS = 1000;
/** @brief Silence after which a peer is considered disconnected. */
inline constexpr std::uint32_t TIMEOUT_MS = 5000;

/**
 * @brief Identifies the payload carried by a datagram.
 *
 * 0x01-0x0F: session, both directions.
 * 0x10-0x1F: game, client to server.
 * 0x20-0x3F: game, server to client.
 */
enum class PacketType : std::uint8_t {
  /** @brief Never sent; a datagram with this type is rejected. */
  Invalid = 0x00,

  ConnectRequest = 0x01,
  ConnectAccept = 0x02,
  ConnectReject = 0x03,
  Disconnect = 0x04,
  Ping = 0x05,
  Pong = 0x06,
  Ack = 0x07,

  PlayerInput = 0x10,

  GameStart = 0x20,
  PlayerJoined = 0x21,
  PlayerLeft = 0x22,
  EntitySpawn = 0x23,
  EntityDestroy = 0x24,
  WorldSnapshot = 0x25,
  PlayerState = 0x26,
  GameOver = 0x27,
};

/** @brief Bits of PacketHeader::flags. Unknown bits are ignored. */
namespace PacketFlag {
/** @brief The receiver must answer with an ACK carrying this sequence. */
inline constexpr std::uint8_t RELIABLE = 0x01;
}  // namespace PacketFlag

/** @brief Bits of PlayerInput::inputMask. */
namespace InputFlag {
inline constexpr std::uint8_t MOVE_UP = 0x01;
inline constexpr std::uint8_t MOVE_DOWN = 0x02;
inline constexpr std::uint8_t MOVE_LEFT = 0x04;
inline constexpr std::uint8_t MOVE_RIGHT = 0x08;
inline constexpr std::uint8_t SHOOT = 0x10;
/**
 * @brief Every defined bit. The bits are contiguous, so a mask is valid
 *        exactly when it is not above ALL.
 */
inline constexpr std::uint8_t ALL = 0x1F;
}  // namespace InputFlag

/** @brief Kind of an entity, so the client knows which sprite to draw. */
enum class EntityType : std::uint8_t {
  Player = 0,
  Bydos = 1,
  PlayerMissile = 2,
  EnemyMissile = 3,
  PowerUp = 4,
  Obstacle = 5,
};

/** @brief Why an entity left the world. */
enum class DestroyReason : std::uint8_t {
  Killed = 0,
  OutOfScreen = 1,
  Collected = 2,
};

/** @brief Why a player left the game. */
enum class DisconnectReason : std::uint8_t {
  Quit = 0,
  Timeout = 1,
  Kicked = 2,
  ServerShutdown = 3,
};

/** @brief Why the server refused a CONNECT_REQUEST. */
enum class RejectReason : std::uint8_t {
  ServerFull = 0,
  GameInProgress = 1,
  VersionMismatch = 2,
  InvalidName = 3,
};

/** @brief Outcome of a game. */
enum class GameResult : std::uint8_t {
  Victory = 0,
  Defeat = 1,
};

/**
 * @brief Whether packets of this type are sent with the RELIABLE flag.
 *
 * @param type Type of the packet.
 * @return true if the receiver must acknowledge it.
 */
[[nodiscard]] constexpr bool isReliable(PacketType type) noexcept {
  switch (type) {
    case PacketType::ConnectAccept:
    case PacketType::GameStart:
    case PacketType::PlayerJoined:
    case PacketType::PlayerLeft:
    case PacketType::EntitySpawn:
    case PacketType::EntityDestroy:
    case PacketType::PlayerState:
    case PacketType::GameOver:
      return true;
    default:
      return false;
  }
}

/** @brief Thrown when asked to encode a packet that breaks the protocol. */
class ProtocolError : public std::runtime_error {
 public:
  /**
   * @brief Creates the error.
   *
   * @param message Description of the error.
   */
  explicit ProtocolError(const std::string& message)
      : std::runtime_error("Protocol error: " + message) {}
};

}  // namespace rtype::protocol
