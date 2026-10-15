#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <random>
#include <span>
#include <variant>
#include <vector>

#include "common/protocol/PacketCodec.hpp"

using namespace rtype::protocol;

namespace {

template <typename T>
T roundTrip(const T& payload, std::uint32_t sequence = 1) {
  const std::vector<std::byte> datagram = encode(payload, sequence);
  Packet packet;
  EXPECT_EQ(decode(datagram, packet), DecodeError::None);
  EXPECT_EQ(packet.header.type, T::TYPE);
  EXPECT_EQ(packet.header.sequence, sequence);
  EXPECT_EQ(packet.header.isReliable(), isReliable(T::TYPE));
  EXPECT_EQ(packet.header.payloadSize, datagram.size() - HEADER_SIZE);
  EXPECT_TRUE(std::holds_alternative<T>(packet.payload));
  return std::get<T>(packet.payload);
}

DecodeError decodeBytes(const std::vector<std::byte>& datagram) {
  Packet packet;
  return decode(datagram, packet);
}

}  // namespace

TEST(protocol, header_layout_is_big_endian) {
  const std::vector<std::byte> datagram =
      encode(Ping{.timestampMs = 0x01020304}, 0xAABBCCDD);
  const std::vector<std::byte> expected = {
      std::byte{0x52}, std::byte{0x54},  // magic "RT"
      std::byte{0x01},                   // version
      std::byte{0x05},                   // type PING
      std::byte{0x00},                   // flags
      std::byte{0x00},                   // reserved
      std::byte{0x00}, std::byte{0x04},  // payload size
      std::byte{0xAA}, std::byte{0xBB}, std::byte{0xCC}, std::byte{0xDD},
      std::byte{0x01}, std::byte{0x02}, std::byte{0x03}, std::byte{0x04},
  };
  EXPECT_EQ(datagram, expected);
}

TEST(protocol, reliable_flag_follows_type) {
  const auto datagram = encode(EntityDestroy{.entityId = 3}, 0);
  EXPECT_EQ(datagram[4], std::byte{PacketFlag::RELIABLE});
}

TEST(protocol, round_trip_session_packets) {
  EXPECT_EQ(roundTrip(ConnectRequest{.playerName = "Seenau"}).playerName,
            "Seenau");
  const auto accept = roundTrip(ConnectAccept{.playerId = 3, .tickRate = 60});
  EXPECT_EQ(accept.playerId, 3);
  EXPECT_EQ(accept.tickRate, 60);
  EXPECT_EQ(roundTrip(ConnectReject{.reason = RejectReason::ServerFull}).reason,
            RejectReason::ServerFull);
  EXPECT_EQ(roundTrip(Disconnect{.reason = DisconnectReason::Quit}).reason,
            DisconnectReason::Quit);
  EXPECT_EQ(roundTrip(Ping{.timestampMs = 123}).timestampMs, 123U);
  EXPECT_EQ(roundTrip(Pong{.timestampMs = 456}).timestampMs, 456U);
  EXPECT_EQ(roundTrip(Ack{.sequence = 789}).sequence, 789U);
}

TEST(protocol, round_trip_game_packets) {
  const auto input = roundTrip(PlayerInput{
      .clientTick = 42,
      .inputMask = InputFlag::MOVE_UP | InputFlag::SHOOT,
  });
  EXPECT_EQ(input.clientTick, 42U);
  EXPECT_EQ(input.inputMask, InputFlag::MOVE_UP | InputFlag::SHOOT);

  EXPECT_EQ(roundTrip(GameStart{.serverTick = 7}).serverTick, 7U);

  const auto joined = roundTrip(PlayerJoined{.playerId = 1, .playerName = "B"});
  EXPECT_EQ(joined.playerId, 1);
  EXPECT_EQ(joined.playerName, "B");

  const auto left =
      roundTrip(PlayerLeft{.playerId = 2, .reason = DisconnectReason::Timeout});
  EXPECT_EQ(left.reason, DisconnectReason::Timeout);

  const auto spawn = roundTrip(EntitySpawn{
      .entityId = 99,
      .entityType = EntityType::Bydos,
      .ownerId = NO_OWNER,
      .x = 1920.5F,
      .y = -12.25F,
      .velocityX = -200.0F,
      .velocityY = 0.0F,
  });
  EXPECT_EQ(spawn.entityId, 99U);
  EXPECT_EQ(spawn.entityType, EntityType::Bydos);
  EXPECT_EQ(spawn.ownerId, NO_OWNER);
  EXPECT_FLOAT_EQ(spawn.x, 1920.5F);
  EXPECT_FLOAT_EQ(spawn.y, -12.25F);
  EXPECT_FLOAT_EQ(spawn.velocityX, -200.0F);

  const auto destroy = roundTrip(
      EntityDestroy{.entityId = 5, .reason = DestroyReason::OutOfScreen});
  EXPECT_EQ(destroy.reason, DestroyReason::OutOfScreen);

  const auto state = roundTrip(
      PlayerState{.playerId = 0, .health = 3, .lives = 2, .score = 79500});
  EXPECT_EQ(state.score, 79500U);

  const auto over =
      roundTrip(GameOver{.result = GameResult::Victory, .finalScore = 10});
  EXPECT_EQ(over.result, GameResult::Victory);
  EXPECT_EQ(over.finalScore, 10U);
}

TEST(protocol, round_trip_full_snapshot) {
  WorldSnapshot snapshot{.serverTick = 1000, .entities = {}};
  for (std::uint32_t id = 0; id < MAX_SNAPSHOT_ENTITIES; ++id) {
    snapshot.entities.push_back({
        .entityId = id,
        .x = static_cast<float>(id),
        .y = 2.0F,
        .velocityX = 3.0F,
        .velocityY = 4.0F,
    });
  }
  const auto datagram = encode(snapshot, 0);
  EXPECT_LE(datagram.size(), MAX_DATAGRAM_SIZE);
  const auto decoded = roundTrip(snapshot);
  ASSERT_EQ(decoded.entities.size(), MAX_SNAPSHOT_ENTITIES);
  EXPECT_EQ(decoded.entities.back().entityId, MAX_SNAPSHOT_ENTITIES - 1);
  EXPECT_FLOAT_EQ(decoded.entities.back().velocityY, 4.0F);
}

TEST(protocol, encode_rejects_invalid_payloads) {
  EXPECT_THROW((void)encode(ConnectRequest{.playerName = ""}, 0),
               ProtocolError);
  EXPECT_THROW(
      (void)encode(ConnectRequest{.playerName = "a name way too long"}, 0),
      ProtocolError);
  EXPECT_THROW((void)encode(ConnectAccept{.playerId = MAX_PLAYERS}, 0),
               ProtocolError);
  EXPECT_THROW((void)encode(PlayerInput{.clientTick = 0, .inputMask = 0x80}, 0),
               ProtocolError);
  WorldSnapshot snapshot;
  snapshot.entities.resize(MAX_SNAPSHOT_ENTITIES + 1);
  EXPECT_THROW((void)encode(snapshot, 0), ProtocolError);
}

TEST(protocol, decode_rejects_bad_headers) {
  auto datagram = encode(Ping{.timestampMs = 1}, 0);

  EXPECT_EQ(decodeBytes({datagram.begin(), datagram.begin() + 11}),
            DecodeError::TooShort);
  EXPECT_EQ(decodeBytes(std::vector<std::byte>(MAX_DATAGRAM_SIZE + 1)),
            DecodeError::TooLarge);

  auto badMagic = datagram;
  badMagic[0] = std::byte{0x00};
  EXPECT_EQ(decodeBytes(badMagic), DecodeError::BadMagic);

  auto badVersion = datagram;
  badVersion[2] = std::byte{0x02};
  Packet packet;
  EXPECT_EQ(decode(badVersion, packet), DecodeError::VersionMismatch);
  EXPECT_EQ(packet.header.version, 2);

  auto unknownType = datagram;
  unknownType[3] = std::byte{0x7F};
  EXPECT_EQ(decodeBytes(unknownType), DecodeError::UnknownType);

  auto trailing = datagram;
  trailing.push_back(std::byte{0});
  EXPECT_EQ(decodeBytes(trailing), DecodeError::SizeMismatch);
}

TEST(protocol, decode_rejects_malformed_payloads) {
  // Trailing byte, with a payloadSize that admits it.
  auto trailing = encode(Ping{.timestampMs = 1}, 0);
  trailing.push_back(std::byte{0});
  trailing[7] = std::byte{5};
  EXPECT_EQ(decodeBytes(trailing), DecodeError::MalformedPayload);

  // Enum value out of range.
  auto badEnum = encode(Disconnect{.reason = DisconnectReason::Quit}, 0);
  badEnum[HEADER_SIZE] = std::byte{0x42};
  EXPECT_EQ(decodeBytes(badEnum), DecodeError::MalformedPayload);

  // Undefined input bit.
  auto badInput = encode(PlayerInput{.clientTick = 0, .inputMask = 0}, 0);
  badInput.back() = std::byte{0x80};
  EXPECT_EQ(decodeBytes(badInput), DecodeError::MalformedPayload);

  // NaN position.
  const EntitySpawn zeroSpawn;
  auto nanSpawn = encode(zeroSpawn, 0);
  for (std::size_t index = HEADER_SIZE + 6; index < HEADER_SIZE + 10; ++index) {
    nanSpawn[index] = std::byte{0xFF};
  }
  EXPECT_EQ(decodeBytes(nanSpawn), DecodeError::MalformedPayload);

  // Snapshot announcing more entities than it carries.
  auto lyingSnapshot =
      encode(WorldSnapshot{.serverTick = 0, .entities = {{}}}, 0);
  lyingSnapshot[HEADER_SIZE + 5] = std::byte{2};
  EXPECT_EQ(decodeBytes(lyingSnapshot), DecodeError::MalformedPayload);

  // Name length beyond the datagram.
  auto longName = encode(ConnectRequest{.playerName = "abc"}, 0);
  longName[HEADER_SIZE] = std::byte{10};
  EXPECT_EQ(decodeBytes(longName), DecodeError::MalformedPayload);
}

TEST(protocol, decode_survives_random_bytes) {
  // Fixed seed on purpose: a failing run must be reproducible.
  std::mt19937 generator(42);  // NOLINT(bugprone-random-generator-seed)
  std::uniform_int_distribution<int> byteValue(0, 255);
  std::uniform_int_distribution<std::size_t> length(0, MAX_DATAGRAM_SIZE + 8);
  std::uniform_int_distribution<int> typeValue(0, 0x30);

  for (int iteration = 0; iteration < 20000; ++iteration) {
    std::vector<std::byte> datagram(length(generator));
    for (std::byte& value : datagram) {
      value = static_cast<std::byte>(byteValue(generator));
    }
    // Make most of them pass the header checks to exercise payload parsing.
    if (datagram.size() >= HEADER_SIZE && iteration % 4 != 0) {
      const std::size_t payloadSize = datagram.size() - HEADER_SIZE;
      datagram[0] = std::byte{0x52};
      datagram[1] = std::byte{0x54};
      datagram[2] = std::byte{VERSION};
      datagram[3] = static_cast<std::byte>(typeValue(generator));
      datagram[6] = static_cast<std::byte>(payloadSize >> 8U);
      datagram[7] = static_cast<std::byte>(payloadSize);
    }
    Packet packet;
    (void)decode(datagram, packet);
  }
  SUCCEED();
}
