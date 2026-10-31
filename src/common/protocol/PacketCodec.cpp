#include <ampersand/net/BinaryReader.hpp>
#include <ampersand/net/BinaryWriter.hpp>
#include <cstddef>
#include <cstdint>
#include <rtype/common/protocol/PacketCodec.hpp>
#include <rtype/common/protocol/PacketHeader.hpp>
#include <rtype/common/protocol/Packets.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace rtype::protocol {

using ampersand::net::BinaryReader;
using ampersand::net::BinaryWriter;

namespace {

/** @brief Offset of payloadSize in the header, patched after encoding. */
constexpr std::size_t PAYLOAD_SIZE_OFFSET = 6;

bool isPlayerId(std::uint8_t playerId) { return playerId < MAX_PLAYERS; }

void checkPlayerName(const std::string& name) {
  if (name.empty() || name.size() > MAX_PLAYER_NAME_LENGTH) {
    throw ProtocolError("player name must be 1 to 16 bytes");
  }
}

/** @brief Throws if a party code is too long to be sent. */
void checkPartyCode(const std::string& code) {
  if (code.size() > MAX_PARTY_CODE_LENGTH) {
    throw ProtocolError("party code must be 0 to 8 bytes");
  }
}

void checkPlayerId(std::uint8_t playerId) {
  if (!isPlayerId(playerId)) {
    throw ProtocolError("player id out of range");
  }
}

void checkRoomId(std::uint32_t roomId) {
  if (roomId == 0) {
    throw ProtocolError("room id 0 is not a room");
  }
}

// The famous writes functions :)

void writePayload(BinaryWriter& writer, const ConnectRequest& payload) {
  checkPlayerName(payload.playerName);
  writer.writeString(payload.playerName);
}

void writePayload(BinaryWriter& writer, const ConnectAccept& payload) {
  if (payload.tickRate == 0) {
    throw ProtocolError("tick rate must not be 0");
  }
  writer.write(payload.tickRate);
}

void writePayload(BinaryWriter& writer, const ConnectReject& payload) {
  writer.writeEnum(payload.reason);
}

void writePayload(BinaryWriter& writer, const Disconnect& payload) {
  writer.writeEnum(payload.reason);
}

void writePayload(BinaryWriter& writer, const Ping& payload) {
  writer.write(payload.timestampMs);
}

void writePayload(BinaryWriter& writer, const Pong& payload) {
  writer.write(payload.timestampMs);
}

void writePayload(BinaryWriter& writer, const Ack& payload) {
  writer.write(payload.sequence);
}

void writePayload(BinaryWriter& writer, const PlayerInput& payload) {
  if (payload.inputMask > InputFlag::ALL) {
    throw ProtocolError("undefined input bits");
  }
  writer.write(payload.clientTick);
  writer.write(payload.inputMask);
}

void writePayload(BinaryWriter& writer, const GameStart& payload) {
  writer.write(payload.serverTick);
}

void writePayload(BinaryWriter& writer, const PlayerJoined& payload) {
  checkPlayerId(payload.playerId);
  checkPlayerName(payload.playerName);
  writer.write(payload.playerId);
  writer.writeString(payload.playerName);
}

void writePayload(BinaryWriter& writer, const PlayerLeft& payload) {
  checkPlayerId(payload.playerId);
  writer.write(payload.playerId);
  writer.writeEnum(payload.reason);
}

void writePayload(BinaryWriter& writer, const EntitySpawn& payload) {
  if (payload.ownerId != NO_OWNER) {
    checkPlayerId(payload.ownerId);
  }
  writer.write(payload.entityId);
  writer.writeEnum(payload.entityType);
  writer.write(payload.ownerId);
  writer.writeFloat(payload.x);
  writer.writeFloat(payload.y);
  writer.writeFloat(payload.velocityX);
  writer.writeFloat(payload.velocityY);
}

void writePayload(BinaryWriter& writer, const EntityDestroy& payload) {
  writer.write(payload.entityId);
  writer.writeEnum(payload.reason);
}

void writePayload(BinaryWriter& writer, const WorldSnapshot& payload) {
  if (payload.entities.size() > MAX_SNAPSHOT_ENTITIES) {
    throw ProtocolError("too many entities in one snapshot");
  }
  writer.write(payload.serverTick);
  writer.write(static_cast<std::uint16_t>(payload.entities.size()));
  for (const EntityState& entity : payload.entities) {
    writer.write(entity.entityId);
    writer.writeFloat(entity.x);
    writer.writeFloat(entity.y);
    writer.writeFloat(entity.velocityX);
    writer.writeFloat(entity.velocityY);
  }
}

void writePayload(BinaryWriter& writer, const PlayerState& payload) {
  checkPlayerId(payload.playerId);
  writer.write(payload.playerId);
  writer.write(payload.health);
  writer.write(payload.lives);
  writer.write(payload.score);
}

void writePayload(BinaryWriter& writer, const GameOver& payload) {
  writer.writeEnum(payload.result);
  writer.write(payload.finalScore);
}

void writePayload(BinaryWriter& /*writer*/, const RoomList& /*payload*/) {}

void writePayload(BinaryWriter& writer, const RoomCreate& payload) {
  writer.writeEnum(payload.visibility);
}

void writePayload(BinaryWriter& writer, const RoomJoin& payload) {
  checkPartyCode(payload.code);
  if (payload.roomId != 0 && !payload.code.empty()) {
    throw ProtocolError("join a room by id or by code, not both");
  }
  writer.write(payload.roomId);
  writer.writeString(payload.code);
}

void writePayload(BinaryWriter& /*writer*/, const RoomLeave& /*payload*/) {}

void writePayload(BinaryWriter& writer, const RoomListReply& payload) {
  if (payload.rooms.size() > MAX_LISTED_ROOMS) {
    throw ProtocolError("too many rooms in one list");
  }
  writer.write(static_cast<std::uint8_t>(payload.rooms.size()));
  for (const RoomInfo& room : payload.rooms) {
    checkRoomId(room.roomId);
    if (room.players > MAX_PLAYERS) {
      throw ProtocolError("more players than a room can hold");
    }
    writer.write(room.roomId);
    writer.write(room.players);
  }
}

void writePayload(BinaryWriter& writer, const RoomJoined& payload) {
  checkRoomId(payload.roomId);
  checkPlayerId(payload.playerId);
  checkPartyCode(payload.code);
  writer.write(payload.roomId);
  writer.write(payload.playerId);
  writer.writeString(payload.code);
}

void writePayload(BinaryWriter& writer, const RoomReject& payload) {
  writer.writeEnum(payload.reason);
}

void writePayload(BinaryWriter& /*writer*/, const RoomLeft& /*payload*/) {}

// And the reading functions

bool readPlayerId(BinaryReader& reader, std::uint8_t& playerId) {
  return reader.read(playerId) && isPlayerId(playerId);
}

bool readPlayerName(BinaryReader& reader, std::string& name) {
  return reader.readString(name, MAX_PLAYER_NAME_LENGTH) && !name.empty();
}

bool readPayload(BinaryReader& reader, ConnectRequest& payload) {
  return readPlayerName(reader, payload.playerName);
}

bool readPayload(BinaryReader& reader, ConnectAccept& payload) {
  return reader.read(payload.tickRate) && payload.tickRate != 0;
}

bool readRejectReason(BinaryReader& reader, RejectReason& reason) {
  return reader.readEnum(reason, RejectReason::RoomNotFound);
}

bool readPayload(BinaryReader& reader, ConnectReject& payload) {
  return readRejectReason(reader, payload.reason);
}

bool readPayload(BinaryReader& reader, Disconnect& payload) {
  return reader.readEnum(payload.reason, DisconnectReason::ServerShutdown);
}

bool readPayload(BinaryReader& reader, Ping& payload) {
  return reader.read(payload.timestampMs);
}

bool readPayload(BinaryReader& reader, Pong& payload) {
  return reader.read(payload.timestampMs);
}

bool readPayload(BinaryReader& reader, Ack& payload) {
  return reader.read(payload.sequence);
}

bool readPayload(BinaryReader& reader, PlayerInput& payload) {
  return reader.read(payload.clientTick) && reader.read(payload.inputMask) &&
         payload.inputMask <= InputFlag::ALL;
}

bool readPayload(BinaryReader& reader, GameStart& payload) {
  return reader.read(payload.serverTick);
}

bool readPayload(BinaryReader& reader, PlayerJoined& payload) {
  return readPlayerId(reader, payload.playerId) &&
         readPlayerName(reader, payload.playerName);
}

bool readPayload(BinaryReader& reader, PlayerLeft& payload) {
  return readPlayerId(reader, payload.playerId) &&
         reader.readEnum(payload.reason, DisconnectReason::ServerShutdown);
}

bool readPayload(BinaryReader& reader, EntitySpawn& payload) {
  return reader.read(payload.entityId) &&
         reader.readEnum(payload.entityType, common::EntityKind::Obstacle) &&
         reader.read(payload.ownerId) &&
         (payload.ownerId == NO_OWNER || isPlayerId(payload.ownerId)) &&
         reader.readFloat(payload.x) && reader.readFloat(payload.y) &&
         reader.readFloat(payload.velocityX) &&
         reader.readFloat(payload.velocityY);
}

bool readPayload(BinaryReader& reader, EntityDestroy& payload) {
  return reader.read(payload.entityId) &&
         reader.readEnum(payload.reason, common::DeathReason::Collected);
}

bool readPayload(BinaryReader& reader, WorldSnapshot& payload) {
  std::uint16_t count = 0;
  if (!reader.read(payload.serverTick) || !reader.read(count) ||
      count > MAX_SNAPSHOT_ENTITIES ||
      reader.remaining() != count * ENTITY_STATE_SIZE) {
    return false;
  }
  payload.entities.resize(count);
  for (EntityState& entity : payload.entities) {
    if (!reader.read(entity.entityId) || !reader.readFloat(entity.x) ||
        !reader.readFloat(entity.y) || !reader.readFloat(entity.velocityX) ||
        !reader.readFloat(entity.velocityY)) {
      return false;
    }
  }
  return true;
}

bool readPayload(BinaryReader& reader, PlayerState& payload) {
  return readPlayerId(reader, payload.playerId) &&
         reader.read(payload.health) && reader.read(payload.lives) &&
         reader.read(payload.score);
}

bool readPayload(BinaryReader& reader, GameOver& payload) {
  return reader.readEnum(payload.result, GameResult::Defeat) &&
         reader.read(payload.finalScore);
}

bool readPayload(BinaryReader& /*reader*/, RoomList& /*payload*/) {
  return true;
}

bool readPayload(BinaryReader& reader, RoomCreate& payload) {
  return reader.readEnum(payload.visibility, RoomVisibility::Private);
}

bool readPayload(BinaryReader& reader, RoomJoin& payload) {
  return reader.read(payload.roomId) &&
         reader.readString(payload.code, MAX_PARTY_CODE_LENGTH) &&
         (payload.roomId == 0 || payload.code.empty());
}

bool readPayload(BinaryReader& /*reader*/, RoomLeave& /*payload*/) {
  return true;
}

bool readPayload(BinaryReader& reader, RoomListReply& payload) {
  std::uint8_t count = 0;
  if (!reader.read(count) || count > MAX_LISTED_ROOMS ||
      reader.remaining() != count * ROOM_INFO_SIZE) {
    return false;
  }
  payload.rooms.resize(count);
  for (RoomInfo& room : payload.rooms) {
    if (!reader.read(room.roomId) || room.roomId == 0 ||
        !reader.read(room.players) || room.players > MAX_PLAYERS) {
      return false;
    }
  }
  return true;
}

bool readPayload(BinaryReader& reader, RoomJoined& payload) {
  return reader.read(payload.roomId) && payload.roomId != 0 &&
         readPlayerId(reader, payload.playerId) &&
         reader.readString(payload.code, MAX_PARTY_CODE_LENGTH);
}

bool readPayload(BinaryReader& reader, RoomReject& payload) {
  return readRejectReason(reader, payload.reason);
}

bool readPayload(BinaryReader& /*reader*/, RoomLeft& /*payload*/) {
  return true;
}

/**
 * @brief Reads a payload of type T, which must use every byte left.
 */
template <typename T>
DecodeError decodeAs(BinaryReader& reader, Payload& out) {
  T payload{};
  if (!readPayload(reader, payload) || !reader.atEnd()) {
    return DecodeError::MalformedPayload;
  }
  out = std::move(payload);
  return DecodeError::None;
}

DecodeError decodePayload(PacketType type, BinaryReader& reader, Payload& out) {
  switch (type) {
    case PacketType::ConnectRequest:
      return decodeAs<ConnectRequest>(reader, out);
    case PacketType::ConnectAccept:
      return decodeAs<ConnectAccept>(reader, out);
    case PacketType::ConnectReject:
      return decodeAs<ConnectReject>(reader, out);
    case PacketType::Disconnect:
      return decodeAs<Disconnect>(reader, out);
    case PacketType::Ping:
      return decodeAs<Ping>(reader, out);
    case PacketType::Pong:
      return decodeAs<Pong>(reader, out);
    case PacketType::Ack:
      return decodeAs<Ack>(reader, out);
    case PacketType::PlayerInput:
      return decodeAs<PlayerInput>(reader, out);
    case PacketType::GameStart:
      return decodeAs<GameStart>(reader, out);
    case PacketType::PlayerJoined:
      return decodeAs<PlayerJoined>(reader, out);
    case PacketType::PlayerLeft:
      return decodeAs<PlayerLeft>(reader, out);
    case PacketType::EntitySpawn:
      return decodeAs<EntitySpawn>(reader, out);
    case PacketType::EntityDestroy:
      return decodeAs<EntityDestroy>(reader, out);
    case PacketType::WorldSnapshot:
      return decodeAs<WorldSnapshot>(reader, out);
    case PacketType::PlayerState:
      return decodeAs<PlayerState>(reader, out);
    case PacketType::GameOver:
      return decodeAs<GameOver>(reader, out);
    case PacketType::RoomList:
      return decodeAs<RoomList>(reader, out);
    case PacketType::RoomCreate:
      return decodeAs<RoomCreate>(reader, out);
    case PacketType::RoomJoin:
      return decodeAs<RoomJoin>(reader, out);
    case PacketType::RoomLeave:
      return decodeAs<RoomLeave>(reader, out);
    case PacketType::RoomListReply:
      return decodeAs<RoomListReply>(reader, out);
    case PacketType::RoomJoined:
      return decodeAs<RoomJoined>(reader, out);
    case PacketType::RoomReject:
      return decodeAs<RoomReject>(reader, out);
    case PacketType::RoomLeft:
      return decodeAs<RoomLeft>(reader, out);
    case PacketType::Invalid:
      break;
  }
  return DecodeError::UnknownType;
}

}  // namespace

PacketType typeOf(const Payload& payload) {
  return std::visit(
      [](const auto& value) { return std::decay_t<decltype(value)>::TYPE; },
      payload);
}

std::vector<std::byte> encode(const Payload& payload, std::uint32_t sequence) {
  const PacketType type = typeOf(payload);
  std::vector<std::byte> buffer;
  buffer.reserve(MAX_DATAGRAM_SIZE);
  BinaryWriter writer(buffer);

  writer.write(MAGIC);
  writer.write(VERSION);
  writer.writeEnum(type);
  writer.write(isReliable(type) ? PacketFlag::RELIABLE : std::uint8_t{0});
  writer.write(std::uint8_t{0});
  writer.write(std::uint16_t{0});
  writer.write(sequence);

  std::visit([&writer](const auto& value) { writePayload(writer, value); },
             payload);

  const std::size_t payloadSize = buffer.size() - HEADER_SIZE;
  if (payloadSize > MAX_PAYLOAD_SIZE) {
    throw ProtocolError("payload larger than MAX_PAYLOAD_SIZE");
  }
  buffer[PAYLOAD_SIZE_OFFSET] = static_cast<std::byte>(payloadSize >> 8U);
  buffer[PAYLOAD_SIZE_OFFSET + 1] = static_cast<std::byte>(payloadSize);
  return buffer;
}

DecodeError decode(std::span<const std::byte> datagram, Packet& out) {
  if (datagram.size() < HEADER_SIZE) {
    return DecodeError::TooShort;
  }
  if (datagram.size() > MAX_DATAGRAM_SIZE) {
    return DecodeError::TooLarge;
  }

  BinaryReader reader(datagram);
  PacketHeader header;
  std::uint8_t rawType = 0;
  // The size was checked above, so the header reads cannot fail.
  (void)reader.read(header.magic);
  (void)reader.read(header.version);
  (void)reader.read(rawType);
  (void)reader.read(header.flags);
  (void)reader.read(header.reserved);
  (void)reader.read(header.payloadSize);
  (void)reader.read(header.sequence);
  header.type = static_cast<PacketType>(rawType);

  if (header.magic != MAGIC) {
    return DecodeError::BadMagic;
  }
  if (header.version != VERSION) {
    out.header = header;
    return DecodeError::VersionMismatch;
  }
  if (header.payloadSize != reader.remaining()) {
    return DecodeError::SizeMismatch;
  }

  Payload payload;
  const DecodeError error = decodePayload(header.type, reader, payload);
  if (error == DecodeError::None) {
    out.header = header;
    out.payload = std::move(payload);
  }
  return error;
}

std::string_view toString(DecodeError error) noexcept {
  switch (error) {
    case DecodeError::None:
      return "none";
    case DecodeError::TooShort:
      return "datagram shorter than the header";
    case DecodeError::TooLarge:
      return "datagram larger than MAX_DATAGRAM_SIZE";
    case DecodeError::BadMagic:
      return "bad magic number";
    case DecodeError::VersionMismatch:
      return "protocol version mismatch";
    case DecodeError::SizeMismatch:
      return "payload size does not match the datagram";
    case DecodeError::UnknownType:
      return "unknown packet type";
    case DecodeError::MalformedPayload:
      return "malformed payload";
  }
  return "unknown error";
}

}  // namespace rtype::protocol
