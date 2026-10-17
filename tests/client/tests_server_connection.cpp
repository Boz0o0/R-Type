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

using namespace rtype::protocol;
using ampersand::net::Endpoint;
using ampersand::net::UdpSocket;
using rtype::client::ServerConnection;
using rtype::common::EntityKind;
using State = ServerConnection::State;

namespace {

const ServerConnection::Clock::time_point START =
    ServerConnection::Clock::now();

/** @brief A fake server: a raw socket that records who talks to it. */
struct FakeServer {
  UdpSocket socket;
  Endpoint client;

  /**
   * @brief Waits for the next valid packet and remembers its sender.
   * @throws std::runtime_error If nothing comes within a second.
   */
  Packet receive() {
    std::array<std::byte, MAX_DATAGRAM_SIZE + 1> buffer{};
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while (std::chrono::steady_clock::now() < deadline) {
      if (const auto length = this->socket.receive(buffer, this->client)) {
        Packet packet;
        if (decode(std::span(buffer).first(*length), packet) ==
            DecodeError::None) {
          return packet;
        }
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    throw std::runtime_error("no packet from the client");
  }

  void send(const Payload& payload, std::uint32_t sequence = 0) {
    this->socket.send(encode(payload, sequence), this->client);
  }
};

/** @brief Calls update() until the state changes or a second has passed. */
void updateUntilStateChanges(ServerConnection& connection,
                             ServerConnection::Clock::time_point now) {
  const State before = connection.state();
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(1);
  while (connection.state() == before &&
         std::chrono::steady_clock::now() < deadline) {
    connection.update(now);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

/** @brief Connects the client, then reads its ACK of CONNECT_ACCEPT. */
void connectToServer(FakeServer& server, ServerConnection& connection) {
  connection.update(START);
  server.receive();
  server.send(ConnectAccept{.playerId = 0}, 1);
  updateUntilStateChanges(connection, START);
  server.receive();
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

TEST(ServerConnection, ConnectsWhenTheServerAccepts) {
  FakeServer server;
  ServerConnection connection("127.0.0.1", server.socket.localPort(), "Bob");
  connection.update(START);
  server.receive();

  server.send(ConnectAccept{.playerId = 2}, 77);
  updateUntilStateChanges(connection, START);

  EXPECT_EQ(connection.state(), State::Connected);
  EXPECT_EQ(connection.playerId(), 2);
  // CONNECT_ACCEPT is reliable, so the client acknowledges it.
  const Packet ack = server.receive();
  ASSERT_TRUE(std::holds_alternative<Ack>(ack.payload));
  EXPECT_EQ(std::get<Ack>(ack.payload).sequence, 77U);
}

TEST(ServerConnection, StopsWhenTheServerRejects) {
  FakeServer server;
  ServerConnection connection("127.0.0.1", server.socket.localPort(), "Bob");
  connection.update(START);
  server.receive();

  server.send(ConnectReject{.reason = RejectReason::ServerFull});
  updateUntilStateChanges(connection, START);

  EXPECT_EQ(connection.state(), State::Rejected);
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

  connection.update(now);
  EXPECT_EQ(connection.state(), State::Unreachable);
}

TEST(ServerConnection, IgnoresGarbageAndOtherSenders) {
  FakeServer server;
  ServerConnection connection("127.0.0.1", server.socket.localPort(), "Bob");
  connection.update(START);
  server.receive();

  server.socket.send(std::span<const std::byte>{}, server.client);
  server.socket.send("garbage", server.client);
  UdpSocket stranger;
  stranger.send(encode(ConnectAccept{.playerId = 3}, 0), server.client);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  connection.update(START);

  EXPECT_EQ(connection.state(), State::Connecting);
  EXPECT_FALSE(connection.playerId().has_value());
}

TEST(ServerConnection, PingsWhileConnectedAndSaysGoodbye) {
  FakeServer server;
  ServerConnection connection("127.0.0.1", server.socket.localPort(), "Bob");
  connection.update(START);
  server.receive();
  server.send(ConnectAccept{.playerId = 0});
  updateUntilStateChanges(connection, START);
  server.receive();  // the ACK

  connection.update(START + ServerConnection::PING_INTERVAL);
  EXPECT_TRUE(std::holds_alternative<Ping>(server.receive().payload));

  connection.disconnect();
  EXPECT_EQ(connection.state(), State::Disconnected);
  const Packet goodbye = server.receive();
  ASSERT_TRUE(std::holds_alternative<Disconnect>(goodbye.payload));
  EXPECT_EQ(std::get<Disconnect>(goodbye.payload).reason,
            DisconnectReason::Quit);
}

TEST(ServerConnection, HandsGamePacketsToTheApp) {
  FakeServer server;
  ServerConnection connection("127.0.0.1", server.socket.localPort(), "Bob");
  connectToServer(server, connection);

  server.send(EntitySpawn{.entityId = 7, .entityType = EntityKind::Enemy}, 5);
  server.send(WorldSnapshot{.serverTick = 3}, 6);
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
  connectToServer(server, connection);

  const EntitySpawn spawn{.entityId = 7, .entityType = EntityKind::Enemy};
  server.send(spawn, 5);
  server.send(spawn, 5);
  server.send(WorldSnapshot{.serverTick = 3}, 6);
  const std::vector<Payload> packets = takeGamePackets(connection, 2);

  ASSERT_EQ(packets.size(), 2U);
  EXPECT_TRUE(std::holds_alternative<EntitySpawn>(packets[0]));
  EXPECT_TRUE(std::holds_alternative<WorldSnapshot>(packets[1]));
  for (int copy = 0; copy < 2; ++copy) {
    const Packet ack = server.receive();
    ASSERT_TRUE(std::holds_alternative<Ack>(ack.payload));
    EXPECT_EQ(std::get<Ack>(ack.payload).sequence, 5U);
  }
}

TEST(ServerConnection, DisconnectsWhenTheServerGoesSilent) {
  FakeServer server;
  ServerConnection connection("127.0.0.1", server.socket.localPort(), "Bob");
  connectToServer(server, connection);

  connection.update(START + ServerConnection::SERVER_TIMEOUT);
  EXPECT_EQ(connection.state(), State::Connected);

  connection.update(START + ServerConnection::SERVER_TIMEOUT +
                    std::chrono::milliseconds(1));
  EXPECT_EQ(connection.state(), State::Disconnected);
}

TEST(ServerConnection, SendsInputOnlyOnceConnected) {
  FakeServer server;
  ServerConnection connection("127.0.0.1", server.socket.localPort(), "Bob");
  connection.update(START);
  connection.sendInput(InputFlag::SHOOT);  // still connecting: not sent
  server.receive();
  server.send(ConnectAccept{.playerId = 0}, 1);
  updateUntilStateChanges(connection, START);
  EXPECT_TRUE(std::holds_alternative<Ack>(server.receive().payload));

  connection.sendInput(InputFlag::MOVE_UP);
  connection.sendInput(InputFlag::MOVE_UP | InputFlag::SHOOT);
  const Packet first = server.receive();
  const Packet second = server.receive();

  const auto* firstInput = std::get_if<PlayerInput>(&first.payload);
  const auto* secondInput = std::get_if<PlayerInput>(&second.payload);
  ASSERT_NE(firstInput, nullptr);
  ASSERT_NE(secondInput, nullptr);
  EXPECT_EQ(firstInput->inputMask, InputFlag::MOVE_UP);
  EXPECT_EQ(secondInput->inputMask, InputFlag::MOVE_UP | InputFlag::SHOOT);
  EXPECT_GT(secondInput->clientTick, firstInput->clientTick);
}
