#pragma once

#include <cstddef>
#include <cstdint>
#include <rtype/common/components/Kind.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <rtype/common/resources/Deaths.hpp>
#include <string>
#include <vector>

/**
 * @file Packets.hpp
 * @brief One struct per packet type, holding the decoded payload.
 *
 * The comment of each struct gives its direction (C = client, S = server),
 * whether it is reliable, and its size on the wire.
 */

namespace rtype::protocol {

/** @brief C->S, unreliable, 1 + n bytes. Asks to enter the lobby. */
struct ConnectRequest {
  static constexpr PacketType TYPE = PacketType::ConnectRequest;
  /** @brief 1 to MAX_PLAYER_NAME_LENGTH bytes of UTF-8. */
  std::string playerName;
};

/** @brief S->C, unreliable, 1 byte. The client is now in the lobby. */
struct ConnectAccept {
  static constexpr PacketType TYPE = PacketType::ConnectAccept;
  /** @brief SERVER_TICK_RATE of the server, for client-side timing. */
  std::uint8_t tickRate{SERVER_TICK_RATE};
};

/** @brief S->C, unreliable, 1 byte. The client may not enter the lobby. */
struct ConnectReject {
  static constexpr PacketType TYPE = PacketType::ConnectReject;
  RejectReason reason{};
};

/** @brief C<->S, unreliable, 1 byte. The sender is leaving. */
struct Disconnect {
  static constexpr PacketType TYPE = PacketType::Disconnect;
  DisconnectReason reason{};
};

/** @brief C<->S, unreliable, 4 bytes. Asks for a PONG. */
struct Ping {
  static constexpr PacketType TYPE = PacketType::Ping;
  /** @brief Sender clock in milliseconds; only the sender reads it. */
  std::uint32_t timestampMs{};
};

/** @brief C<->S, unreliable, 4 bytes. Answer to a PING. */
struct Pong {
  static constexpr PacketType TYPE = PacketType::Pong;
  /** @brief Copied from the PING; round trip = now - timestampMs. */
  std::uint32_t timestampMs{};
};

/** @brief C<->S, unreliable, 4 bytes. Confirms a reliable packet. */
struct Ack {
  static constexpr PacketType TYPE = PacketType::Ack;
  /** @brief Header sequence of the reliable packet received. */
  std::uint32_t sequence{};
};

/** @brief C->S, unreliable, 5 bytes. State of the controls. */
struct PlayerInput {
  static constexpr PacketType TYPE = PacketType::PlayerInput;
  /** @brief Client frame counter; older inputs than the last one are dropped.
   */
  std::uint32_t clientTick{};
  /** @brief InputFlag bits held this frame. */
  std::uint8_t inputMask{};
};

/** @brief S->C, reliable, 4 bytes. The game begins. */
struct GameStart {
  static constexpr PacketType TYPE = PacketType::GameStart;
  /** @brief Server tick at which the game starts. */
  std::uint32_t serverTick{};
};

/** @brief S->C, reliable, 2 + n bytes. A player is in the game. */
struct PlayerJoined {
  static constexpr PacketType TYPE = PacketType::PlayerJoined;
  std::uint8_t playerId{};
  std::string playerName;
};

/** @brief S->C, reliable, 2 bytes. A player left (quit, crash...). */
struct PlayerLeft {
  static constexpr PacketType TYPE = PacketType::PlayerLeft;
  std::uint8_t playerId{};
  DisconnectReason reason{};
};

/** @brief S->C, reliable, 22 bytes. An entity appeared in the world. */
struct EntitySpawn {
  static constexpr PacketType TYPE = PacketType::EntitySpawn;
  /** @brief Server-wide id, never reused during a game. */
  std::uint32_t entityId{};
  /** @brief What it is, so the client knows which sprite to draw. */
  common::EntityKind entityType{};
  /** @brief Player that owns it (ship, missile), or NO_OWNER. */
  std::uint8_t ownerId{NO_OWNER};
  /** @brief Position in world units (pixels of a 1920x1080 screen). */
  float x{};
  float y{};
  /** @brief Velocity in world units per second. */
  float velocityX{};
  float velocityY{};
};

/** @brief S->C, reliable, 5 bytes. An entity left the world. */
struct EntityDestroy {
  static constexpr PacketType TYPE = PacketType::EntityDestroy;
  std::uint32_t entityId{};
  common::DeathReason reason{};
};

/** @brief State of one entity inside a WORLD_SNAPSHOT, 20 bytes. */
struct EntityState {
  std::uint32_t entityId{};
  float x{};
  float y{};
  float velocityX{};
  float velocityY{};
};

/** @brief Size of one EntityState on the wire. */
inline constexpr std::size_t ENTITY_STATE_SIZE = 20;
/** @brief Most entities one WORLD_SNAPSHOT can carry (59). */
inline constexpr std::size_t MAX_SNAPSHOT_ENTITIES =
    (MAX_PAYLOAD_SIZE - 6) / ENTITY_STATE_SIZE;

/** @brief S->C, unreliable, 6 + 20 * n bytes. Positions of entities. */
struct WorldSnapshot {
  static constexpr PacketType TYPE = PacketType::WorldSnapshot;
  /** @brief Server tick of the state; older snapshots are dropped. */
  std::uint32_t serverTick{};
  /** @brief At most MAX_SNAPSHOT_ENTITIES; split bigger worlds. */
  std::vector<EntityState> entities;
};

/** @brief S->C, reliable, 7 bytes. Health, lives and score of a player. */
struct PlayerState {
  static constexpr PacketType TYPE = PacketType::PlayerState;
  std::uint8_t playerId{};
  std::uint8_t health{};
  std::uint8_t lives{};
  std::uint32_t score{};
};

/** @brief S->C, reliable, 5 bytes. The game is over. */
struct GameOver {
  static constexpr PacketType TYPE = PacketType::GameOver;
  GameResult result{};
  std::uint32_t finalScore{};
};

/** @brief C->S, unreliable, 0 bytes. Asks for the public rooms. */
struct RoomList {
  static constexpr PacketType TYPE = PacketType::RoomList;
};

/** @brief C->S, unreliable, 1 byte. Opens a new room and joins it. */
struct RoomCreate {
  static constexpr PacketType TYPE = PacketType::RoomCreate;
  /** @brief A private room gets a code chosen by the server. */
  RoomVisibility visibility{};
};

/**
 * @brief C->S, unreliable, 5 + n bytes. Joins a room.
 *
 * With a code: the private room of this code, opened if there is none.
 * Otherwise with a room id: this public room. With neither: any public room
 * with a free place, opened if they are all full (quick match).
 */
struct RoomJoin {
  static constexpr PacketType TYPE = PacketType::RoomJoin;
  /** @brief Public room to join, or 0. Must be 0 when code is set. */
  std::uint32_t roomId{};
  /** @brief 0 to MAX_PARTY_CODE_LENGTH bytes. */
  std::string code;
};

/** @brief C->S, unreliable, 0 bytes. Leaves the room, back to the lobby. */
struct RoomLeave {
  static constexpr PacketType TYPE = PacketType::RoomLeave;
};

/** @brief One public room inside a ROOM_LIST_REPLY, 5 bytes. */
struct RoomInfo {
  /** @brief Id to send in ROOM_JOIN, never 0. */
  std::uint32_t roomId{};
  /** @brief Players in it, from 0 to MAX_PLAYERS. */
  std::uint8_t players{};
};

/** @brief Size of one RoomInfo on the wire. */
inline constexpr std::size_t ROOM_INFO_SIZE = 5;

/** @brief S->C, unreliable, 1 + 5 * n bytes. The public rooms. */
struct RoomListReply {
  static constexpr PacketType TYPE = PacketType::RoomListReply;
  /** @brief At most MAX_LISTED_ROOMS, oldest room first. */
  std::vector<RoomInfo> rooms;
};

/** @brief S->C, reliable, 6 + n bytes. The client is now in a room. */
struct RoomJoined {
  static constexpr PacketType TYPE = PacketType::RoomJoined;
  /** @brief Id of the room, never 0. */
  std::uint32_t roomId{};
  /** @brief Id given to this client, from 0 to MAX_PLAYERS - 1. */
  std::uint8_t playerId{};
  /** @brief Code of a private room, to give to friends; empty if public. */
  std::string code;
};

/** @brief S->C, unreliable, 1 byte. The client may not join this room. */
struct RoomReject {
  static constexpr PacketType TYPE = PacketType::RoomReject;
  RejectReason reason{};
};

/** @brief S->C, unreliable, 0 bytes. The client is back in the lobby. */
struct RoomLeft {
  static constexpr PacketType TYPE = PacketType::RoomLeft;
};

}  // namespace rtype::protocol
