#include <gtest/gtest.h>

#include <ampersand/net/UdpSocket.hpp>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <rtype/client/ServerConnection.hpp>
#include <rtype/common/protocol/PacketCodec.hpp>
#include <rtype/common/protocol/Packets.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <span>
#include <thread>
#include <variant>

using namespace rtype::protocol;
using ampersand::net::Endpoint;
using ampersand::net::UdpSocket;
using rtype::client::ServerConnection;
using State = ServerConnection::State;

namespace {

const ServerConnection::Clock::time_point START =
    ServerConnection::Clock::now();

/** @brief A fake server: a raw socket that records who talks to it. */
struct FakeServer {
  UdpSocket socket;
  Endpoint client;

  /** @brief Waits for the next valid packet and remembers its sender. */
  std::optional<Packet> receive() {
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
    return std::nullopt;
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

}  // namespace

TEST(ServerConnection, SendsConnectRequestWithThePlayerName) {
  FakeServer server;
  ServerConnection connection("127.0.0.1", server.socket.localPort(), "Bob");

  connection.update(START);
  const auto packet = server.receive();

  ASSERT_TRUE(packet.has_value());
  const auto* request = std::get_if<ConnectRequest>(&packet->payload);
  ASSERT_NE(request, nullptr);
  EXPECT_EQ(request->playerName, "Bob");
  EXPECT_EQ(connection.state(), State::Connecting);
}

TEST(ServerConnection, ConnectsWhenTheServerAccepts) {
  FakeServer server;
  ServerConnection connection("127.0.0.1", server.socket.localPort(), "Bob");
  connection.update(START);
  ASSERT_TRUE(server.receive().has_value());

  server.send(ConnectAccept{.playerId = 2}, 77);
  updateUntilStateChanges(connection, START);

  EXPECT_EQ(connection.state(), State::Connected);
  EXPECT_EQ(connection.playerId(), 2);
  // CONNECT_ACCEPT is reliable, so the client acknowledges it.
  const auto ack = server.receive();
  ASSERT_TRUE(ack.has_value());
  ASSERT_TRUE(std::holds_alternative<Ack>(ack->payload));
  EXPECT_EQ(std::get<Ack>(ack->payload).sequence, 77U);
}

TEST(ServerConnection, StopsWhenTheServerRejects) {
  FakeServer server;
  ServerConnection connection("127.0.0.1", server.socket.localPort(), "Bob");
  connection.update(START);
  ASSERT_TRUE(server.receive().has_value());

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
    const auto packet = server.receive();
    ASSERT_TRUE(packet.has_value());
    EXPECT_TRUE(std::holds_alternative<ConnectRequest>(packet->payload));
  }
  EXPECT_EQ(connection.state(), State::Connecting);

  connection.update(now);
  EXPECT_EQ(connection.state(), State::Unreachable);
}

TEST(ServerConnection, IgnoresGarbageAndOtherSenders) {
  FakeServer server;
  ServerConnection connection("127.0.0.1", server.socket.localPort(), "Bob");
  connection.update(START);
  ASSERT_TRUE(server.receive().has_value());

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
  ASSERT_TRUE(server.receive().has_value());
  server.send(ConnectAccept{.playerId = 0});
  updateUntilStateChanges(connection, START);
  ASSERT_TRUE(server.receive().has_value());  // the ACK

  connection.update(START + ServerConnection::PING_INTERVAL);
  const auto ping = server.receive();
  ASSERT_TRUE(ping.has_value());
  EXPECT_TRUE(std::holds_alternative<Ping>(ping->payload));

  connection.disconnect();
  EXPECT_EQ(connection.state(), State::Disconnected);
  const auto goodbye = server.receive();
  ASSERT_TRUE(goodbye.has_value());
  ASSERT_TRUE(std::holds_alternative<Disconnect>(goodbye->payload));
  EXPECT_EQ(std::get<Disconnect>(goodbye->payload).reason,
            DisconnectReason::Quit);
}
