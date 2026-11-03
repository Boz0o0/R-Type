#include <gtest/gtest.h>

#include <ampersand/net/UdpSocket.hpp>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <rtype/client/ServerConnection.hpp>
#include <rtype/common/components/Kind.hpp>
#include <rtype/common/protocol/PacketCodec.hpp>
#include <rtype/common/protocol/Packets.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <span>
#include <stdexcept>
#include <thread>
#include <utility>
#include <variant>
#include <vector>

#include "FakeServer.hpp"

using namespace rtype::protocol;
using ampersand::net::Endpoint;
using ampersand::net::UdpSocket;
using rtype::client::ServerConnection;
using rtype::common::EntityKind;
using State = ServerConnection::State;
using rtype::client::test::connectToLobby;
using rtype::client::test::enterRoom;
using rtype::client::test::FakeServer;
using rtype::client::test::JOINED_SEQUENCE;
using rtype::client::test::START;
using rtype::client::test::updateUntilStateChanges;

namespace {

/** @brief Calls update() for a while, to read what the server sent. */
void updateFor(ServerConnection& connection,
               ServerConnection::Clock::time_point now) {
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(50);
  while (std::chrono::steady_clock::now() < deadline) {
    connection.update(now);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

/** @brief Calls update() until count game packets came, or a second passed. */
std::vector<Payload> takeGamePackets(ServerConnection& connection,
                                     std::size_t count) {
  std::vector<Payload> packets;
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(1);
  while (packets.size() < count &&
         std::chrono::steady_clock::now() < deadline) {
    connection.update(START);
    for (Payload& packet : connection.takePackets()) {
      packets.push_back(std::move(packet));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return packets;
}

}  // namespace

// Entering the lobby

TEST(ServerConnection, SendsConnectRequestWithThePlayerName) {
  FakeServer server;
  ServerConnection connection("127.0.0.1", server.socket.localPort(), "Bob");

  connection.update(START);
  const Packet packet = server.receive();

  const auto* request = std::get_if<ConnectRequest>(&packet.payload);
  ASSERT_NE(request, nullptr);
  EXPECT_EQ(request->playerName, "Bob");
  EXPECT_EQ(connection.state(), State::Connecting);
}

TEST(ServerConnection, EntersTheLobbyWhenTheServerAccepts) {
  FakeServer server;
  ServerConnection connection("127.0.0.1", server.socket.localPort(), "Bob");
  connection.update(START);
  server.receive();

  server.send(ConnectAccept{});
  updateUntilStateChanges(connection, START);

  EXPECT_EQ(connection.state(), State::InLobby);
  EXPECT_FALSE(connection.hasEnded());
  EXPECT_FALSE(connection.playerId().has_value());
  // The first thing it wants: the rooms.
  EXPECT_TRUE(std::holds_alternative<RoomList>(server.receive().payload));
}

TEST(ServerConnection, StopsWhenTheServerRejects) {
  FakeServer server;
  ServerConnection connection("127.0.0.1", server.socket.localPort(), "Bob");
  connection.update(START);
  server.receive();

  server.send(ConnectReject{.reason = RejectReason::ServerFull});
  updateUntilStateChanges(connection, START);

  EXPECT_EQ(connection.state(), State::Rejected);
  EXPECT_TRUE(connection.hasEnded());
  EXPECT_EQ(connection.rejectReason(), RejectReason::ServerFull);
  EXPECT_FALSE(connection.playerId().has_value());
}

TEST(ServerConnection, RetriesThenGivesUp) {
  FakeServer server;
  ServerConnection connection("127.0.0.1", server.socket.localPort(), "Bob");

  auto now = START;
  for (std::uint32_t attempt = 0;
       attempt < ServerConnection::MAX_CONNECT_ATTEMPTS; ++attempt) {
    connection.update(now);
    // Too early for another request.
    connection.update(now + ServerConnection::CONNECT_RETRY_DELAY / 2);
    now += ServerConnection::CONNECT_RETRY_DELAY;
  }
  for (std::uint32_t attempt = 0;
       attempt < ServerConnection::MAX_CONNECT_ATTEMPTS; ++attempt) {
    EXPECT_TRUE(
        std::holds_alternative<ConnectRequest>(server.receive().payload));
  }
  EXPECT_EQ(connection.state(), State::Connecting);
  EXPECT_FALSE(connection.hasEnded());

  connection.update(now);
  EXPECT_EQ(connection.state(), State::Unreachable);
  EXPECT_TRUE(connection.hasEnded());
}

TEST(ServerConnection, IgnoresGarbageAndOtherSenders) {
  FakeServer server;
  ServerConnection connection("127.0.0.1", server.socket.localPort(), "Bob");
  connection.update(START);
  server.receive();

  server.socket.send(std::span<const std::byte>{}, server.client);
  server.socket.send("garbage", server.client);
  UdpSocket stranger;
  stranger.send(encode(ConnectAccept{}, 0), server.client);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  connection.update(START);

  EXPECT_EQ(connection.state(), State::Connecting);
}

// In the lobby

TEST(ServerConnection, KeepsTheRoomListUpToDate) {
  FakeServer server;
  ServerConnection connection("127.0.0.1", server.socket.localPort(), "Bob");
  connectToLobby(server, connection);
  server.receive<RoomList>();

  server.send(RoomListReply{.rooms = {{.roomId = 2, .players = 3}}});
  updateFor(connection, START);
  ASSERT_EQ(connection.rooms().size(), 1U);
  EXPECT_EQ(connection.rooms()[0].roomId, 2U);
  EXPECT_EQ(connection.rooms()[0].players, 3);

  connection.update(START + ServerConnection::ROOM_LIST_INTERVAL);
  EXPECT_NO_THROW(server.receive<RoomList>());
}

TEST(ServerConnection, IgnoresRoomRequestsOutsideTheLobby) {
  FakeServer server;
  ServerConnection connection("127.0.0.1", server.socket.localPort(), "Bob");

  connection.enterRoom(RoomJoin{});  // still connecting

  EXPECT_EQ(connection.state(), State::Connecting);
}

TEST(ServerConnection, PingsWhileConnectedAndSaysGoodbye) {
  FakeServer server;
  ServerConnection connection("127.0.0.1", server.socket.localPort(), "Bob");
  connectToLobby(server, connection);

  connection.update(START + ServerConnection::PING_INTERVAL);
  EXPECT_NO_THROW(server.receive<Ping>());

  connection.disconnect();
  EXPECT_EQ(connection.state(), State::Disconnected);
  EXPECT_EQ(server.receive<Disconnect>().reason, DisconnectReason::Quit);
}

TEST(ServerConnection, DisconnectsWhenTheServerGoesSilent) {
  FakeServer server;
  ServerConnection connection("127.0.0.1", server.socket.localPort(), "Bob");
  connectToLobby(server, connection);

  connection.update(START + ServerConnection::SERVER_TIMEOUT);
  EXPECT_EQ(connection.state(), State::InLobby);

  connection.update(START + ServerConnection::SERVER_TIMEOUT +
                    std::chrono::milliseconds(1));
  EXPECT_EQ(connection.state(), State::Disconnected);
  EXPECT_TRUE(connection.hasEnded());
}

// Entering and leaving a room

TEST(ServerConnection, AsksForTheRoomUntilTheServerAnswers) {
  FakeServer server;
  ServerConnection connection("127.0.0.1", server.socket.localPort(), "Bob");
  connectToLobby(server, connection);

  connection.enterRoom(RoomJoin{.roomId = 3});
  EXPECT_EQ(connection.state(), State::Joining);
  connection.update(START);
  const auto request = server.receive<RoomJoin>();
  EXPECT_EQ(request.roomId, 3U);
  connection.update(START + ServerConnection::REQUEST_RETRY_DELAY);
  const auto copy = server.receive<RoomJoin>();
  EXPECT_EQ(copy.roomId, 3U);
  EXPECT_EQ(copy.requestId, request.requestId);  // same request, same id

  server.send(
      RoomJoined{.requestId = request.requestId, .roomId = 3, .playerId = 2},
      77);
  updateUntilStateChanges(connection, START);

  EXPECT_EQ(connection.state(), State::InRoom);
  EXPECT_EQ(connection.playerId(), 2);
  ASSERT_TRUE(connection.room().has_value());
  EXPECT_EQ(connection.room()->roomId, 3U);
  // ROOM_JOINED is reliable, so the client acknowledges it.
  EXPECT_EQ(server.receive<Ack>().sequence, 77U);
}

TEST(ServerConnection, GivesEachRoomRequestANewId) {
  FakeServer server;
  ServerConnection connection("127.0.0.1", server.socket.localPort(), "Bob");
  connectToLobby(server, connection);
  connection.enterRoom(RoomJoin{.roomId = 3});
  connection.update(START);
  const auto first = server.receive<RoomJoin>();
  server.send(RoomReject{.requestId = first.requestId,
                         .reason = RejectReason::PartyFull});
  updateUntilStateChanges(connection, START);

  connection.enterRoom(RoomCreate{});
  connection.update(START);

  EXPECT_NE(server.receive<RoomCreate>().requestId, first.requestId);
}

TEST(ServerConnection, SendsTheRoomToCreate) {
  FakeServer server;
  ServerConnection connection("127.0.0.1", server.socket.localPort(), "Bob");
  connectToLobby(server, connection);

  connection.enterRoom(RoomCreate{.visibility = RoomVisibility::Private});
  connection.update(START);

  EXPECT_EQ(server.receive<RoomCreate>().visibility, RoomVisibility::Private);
}

TEST(ServerConnection, StaysInTheLobbyWhenARoomIsRefused) {
  FakeServer server;
  ServerConnection connection("127.0.0.1", server.socket.localPort(), "Bob");
  connectToLobby(server, connection);
  connection.enterRoom(RoomJoin{.roomId = 3});
  connection.update(START);
  const auto request = server.receive<RoomJoin>();

  server.send(RoomReject{.requestId = request.requestId,
                         .reason = RejectReason::PartyFull});
  updateUntilStateChanges(connection, START);

  EXPECT_EQ(connection.state(), State::InLobby);
  EXPECT_FALSE(connection.hasEnded());
  EXPECT_EQ(connection.roomRejectReason(), RejectReason::PartyFull);
  connection.enterRoom(RoomJoin{});
  EXPECT_FALSE(connection.roomRejectReason().has_value());
}

// Answers to an older request come late: the room chosen now is another one.
TEST(ServerConnection, IgnoresTheAnswersToAnotherRequest) {
  FakeServer server;
  ServerConnection connection("127.0.0.1", server.socket.localPort(), "Bob");
  connectToLobby(server, connection);
  connection.enterRoom(RoomJoin{.roomId = 7});
  connection.update(START);
  const auto request = server.receive<RoomJoin>();
  const auto older = static_cast<std::uint8_t>(request.requestId - 1);

  server.send(
      RoomReject{.requestId = older, .reason = RejectReason::PartyFull});
  server.send(RoomJoined{.requestId = older, .roomId = 5, .playerId = 0}, 40);
  updateFor(connection, START);
  EXPECT_EQ(connection.state(), State::Joining);

  server.send(
      RoomJoined{.requestId = request.requestId, .roomId = 7, .playerId = 1},
      41);
  updateUntilStateChanges(connection, START);
  EXPECT_EQ(connection.state(), State::InRoom);
  ASSERT_TRUE(connection.room().has_value());
  EXPECT_EQ(connection.room()->roomId, 7U);
}

TEST(ServerConnection, LeavesTheRoomForTheLobby) {
  FakeServer server;
  ServerConnection connection("127.0.0.1", server.socket.localPort(), "Bob");
  enterRoom(server, connection);
  const auto generation = connection.roomGeneration();

  connection.leaveRoom();
  EXPECT_EQ(connection.state(), State::Leaving);
  EXPECT_NE(connection.roomGeneration(), generation);
  connection.update(START);
  EXPECT_NO_THROW(server.receive<RoomLeave>());

  server.send(RoomLeft{}, JOINED_SEQUENCE + 1);
  updateUntilStateChanges(connection, START);

  EXPECT_EQ(connection.state(), State::InLobby);
  EXPECT_FALSE(connection.room().has_value());
  EXPECT_FALSE(connection.playerId().has_value());
  EXPECT_NO_THROW(server.receive<RoomList>());
}

// The server moved us back to the lobby on its own: we must follow, or the
// world stays frozen while the lobby answers our PINGs forever.
TEST(ServerConnection, GoesBackToTheLobbyWhenTheServerSaysSo) {
  FakeServer server;
  ServerConnection connection("127.0.0.1", server.socket.localPort(), "Bob");
  enterRoom(server, connection);
  const auto generation = connection.roomGeneration();

  server.send(RoomLeft{}, JOINED_SEQUENCE + 5);
  updateUntilStateChanges(connection, START);

  EXPECT_EQ(connection.state(), State::InLobby);
  EXPECT_FALSE(connection.room().has_value());
  EXPECT_NE(connection.roomGeneration(), generation);
  EXPECT_NO_THROW(server.receive<RoomList>());
}

TEST(ServerConnection, IgnoresARoomLeftOlderThanItsRoom) {
  FakeServer server;
  ServerConnection connection("127.0.0.1", server.socket.localPort(), "Bob");
  enterRoom(server, connection);

  // Sent before we joined this room: about the one we left before.
  server.send(RoomLeft{}, JOINED_SEQUENCE - 1);
  updateFor(connection, START);

  EXPECT_EQ(connection.state(), State::InRoom);
}

// While Joining, the request is sent again, and answered by the lobby.
TEST(ServerConnection, KeepsAskingForItsRoomAfterARoomLeft) {
  FakeServer server;
  ServerConnection connection("127.0.0.1", server.socket.localPort(), "Bob");
  connectToLobby(server, connection);
  connection.enterRoom(RoomJoin{.roomId = 3});
  connection.update(START);
  server.receive<RoomJoin>();

  server.send(RoomLeft{}, 50);
  updateFor(connection, START);
  connection.update(START + ServerConnection::REQUEST_RETRY_DELAY);

  EXPECT_EQ(connection.state(), State::Joining);
  EXPECT_EQ(server.receive<RoomJoin>().roomId, 3U);
}

TEST(ServerConnection, DropsTheGamePacketsOfARoomItLeft) {
  FakeServer server;
  ServerConnection connection("127.0.0.1", server.socket.localPort(), "Bob");
  enterRoom(server, connection);
  server.send(EntitySpawn{.entityId = 7, .entityType = EntityKind::Enemy},
              JOINED_SEQUENCE + 1);
  ASSERT_EQ(takeGamePackets(connection, 1).size(), 1U);
  EXPECT_EQ(server.receive<Ack>().sequence, JOINED_SEQUENCE + 1);

  connection.leaveRoom();
  server.send(EntitySpawn{.entityId = 8, .entityType = EntityKind::Enemy},
              JOINED_SEQUENCE + 2);
  updateFor(connection, START);
  EXPECT_TRUE(connection.takePackets().empty());
  // Still acknowledged.
  EXPECT_EQ(server.receive<Ack>().sequence, JOINED_SEQUENCE + 2);

  server.send(RoomLeft{}, JOINED_SEQUENCE + 3);
  server.send(WorldSnapshot{.serverTick = 9}, JOINED_SEQUENCE + 4);
  updateFor(connection, START);
  EXPECT_EQ(connection.state(), State::InLobby);
  EXPECT_TRUE(connection.takePackets().empty());
  EXPECT_EQ(connection.lastServerTick(), 0U);
}

// Packets of the old room still on their way while we join the next one: the
// old snapshot would hide the new ones, the old spawn would be a ghost.
TEST(ServerConnection, KeepsOnlyTheGamePacketsOfTheRoomItJoins) {
  FakeServer server;
  ServerConnection connection("127.0.0.1", server.socket.localPort(), "Bob");
  connectToLobby(server, connection);
  connection.enterRoom(RoomJoin{});
  connection.update(START);
  const auto request = server.receive<RoomJoin>();

  server.send(WorldSnapshot{.serverTick = 900}, 10);  // old room
  server.send(EntitySpawn{.entityId = 1, .entityType = EntityKind::Enemy},
              11);  // old room
  server.send(EntitySpawn{.entityId = 2, .entityType = EntityKind::Ship},
              21);  // new room, before its ROOM_JOINED
  updateFor(connection, START);
  EXPECT_TRUE(connection.takePackets().empty());  // not in the room yet
  server.send(
      RoomJoined{.requestId = request.requestId, .roomId = 4, .playerId = 0},
      20);
  updateUntilStateChanges(connection, START);
  server.send(WorldSnapshot{.serverTick = 12}, 9);  // old room, very late
  server.send(WorldSnapshot{.serverTick = 3}, 22);  // new room
  const std::vector<Payload> packets = takeGamePackets(connection, 2);

  ASSERT_EQ(packets.size(), 2U);
  ASSERT_TRUE(std::holds_alternative<EntitySpawn>(packets[0]));
  EXPECT_EQ(std::get<EntitySpawn>(packets[0]).entityId, 2U);
  ASSERT_TRUE(std::holds_alternative<WorldSnapshot>(packets[1]));
  EXPECT_EQ(std::get<WorldSnapshot>(packets[1]).serverTick, 3U);
  EXPECT_EQ(connection.lastServerTick(), 3U);
  updateFor(connection, START);
  EXPECT_TRUE(connection.takePackets().empty());
}

TEST(ServerConnection, LeavesARoomItNoLongerWants) {
  FakeServer server;
  ServerConnection connection("127.0.0.1", server.socket.localPort(), "Bob");
  connectToLobby(server, connection);

  // A late answer to a request it no longer sends.
  server.send(RoomJoined{.roomId = 5, .playerId = 0}, 9);
  updateUntilStateChanges(connection, START);

  EXPECT_EQ(connection.state(), State::Leaving);
  EXPECT_NO_THROW(server.receive<RoomLeave>());
}

// In a room

TEST(ServerConnection, HandsGamePacketsToTheApp) {
  FakeServer server;
  ServerConnection connection("127.0.0.1", server.socket.localPort(), "Bob");
  enterRoom(server, connection);

  server.send(EntitySpawn{.entityId = 7, .entityType = EntityKind::Enemy},
              JOINED_SEQUENCE + 1);
  server.send(WorldSnapshot{.serverTick = 3}, JOINED_SEQUENCE + 2);
  const std::vector<Payload> packets = takeGamePackets(connection, 2);

  ASSERT_EQ(packets.size(), 2U);
  EXPECT_TRUE(std::holds_alternative<EntitySpawn>(packets[0]));
  EXPECT_TRUE(std::holds_alternative<WorldSnapshot>(packets[1]));
  EXPECT_EQ(connection.lastServerTick(), 3U);
  EXPECT_TRUE(connection.takePackets().empty());
}

// The server resends a reliable packet when our ACK is lost.
TEST(ServerConnection, HandlesAReliablePacketOnceButAcksEveryCopy) {
  FakeServer server;
  ServerConnection connection("127.0.0.1", server.socket.localPort(), "Bob");
  enterRoom(server, connection);

  const EntitySpawn spawn{.entityId = 7, .entityType = EntityKind::Enemy};
  server.send(spawn, JOINED_SEQUENCE + 1);
  server.send(spawn, JOINED_SEQUENCE + 1);
  server.send(WorldSnapshot{.serverTick = 3}, JOINED_SEQUENCE + 2);
  const std::vector<Payload> packets = takeGamePackets(connection, 2);

  ASSERT_EQ(packets.size(), 2U);
  EXPECT_TRUE(std::holds_alternative<EntitySpawn>(packets[0]));
  EXPECT_TRUE(std::holds_alternative<WorldSnapshot>(packets[1]));
  for (int copy = 0; copy < 2; ++copy) {
    EXPECT_EQ(server.receive<Ack>().sequence, JOINED_SEQUENCE + 1);
  }
}

TEST(ServerConnection, SendsInputOnlyInARoom) {
  FakeServer server;
  ServerConnection connection("127.0.0.1", server.socket.localPort(), "Bob");
  connectToLobby(server, connection);
  connection.sendInput(InputFlag::SHOOT);  // in the lobby: not sent
  connection.enterRoom(RoomJoin{});
  connection.update(START);
  const auto request = server.receive<RoomJoin>();
  server.send(
      RoomJoined{.requestId = request.requestId, .roomId = 3, .playerId = 0},
      1);
  updateUntilStateChanges(connection, START);

  connection.sendInput(InputFlag::MOVE_UP);
  connection.sendInput(InputFlag::MOVE_UP | InputFlag::SHOOT);
  const auto first = server.receive<PlayerInput>();
  const auto second = server.receive<PlayerInput>();

  EXPECT_EQ(first.inputMask, InputFlag::MOVE_UP);
  EXPECT_EQ(second.inputMask, InputFlag::MOVE_UP | InputFlag::SHOOT);
  EXPECT_GT(second.clientTick, first.clientTick);
}
