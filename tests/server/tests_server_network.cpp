#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <rtype/common/protocol/PacketCodec.hpp>
#include <rtype/common/protocol/Packets.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <rtype/server/ServerNetwork.hpp>
#include <span>
#include <string>
#include <variant>
#include <vector>

#include "ServerTestHelpers.hpp"

using namespace rtype::protocol;
using rtype::server::ServerNetwork;
using rtype::server::test::receiveWithin;
using rtype::server::test::TestClient;

TEST(ServerNetwork, QueuesDecodedPacketsWithTheirSender) {
  ServerNetwork network(0);
  TestClient client(network);

  client.send(ConnectRequest{.playerName = "Alice"}, 7);
  const auto messages = receiveWithin(network);

  ASSERT_EQ(messages.size(), 1U);
  EXPECT_EQ(messages[0].from.port, client.socket.localPort());
  EXPECT_EQ(messages[0].packet.header.sequence, 7U);
  const auto* request =
      std::get_if<ConnectRequest>(&messages[0].packet.payload);
  ASSERT_NE(request, nullptr);
  EXPECT_EQ(request->playerName, "Alice");
}

TEST(ServerNetwork, DropsInvalidDatagramsAndKeepsRunning) {
  ServerNetwork network(0);
  TestClient client(network);

  std::vector<std::byte> valid = encode(Ping{.timestampMs = 1}, 0);
  std::vector<std::byte> badMagic = valid;
  badMagic[0] = std::byte{0x00};
  std::vector<std::byte> badSize = valid;
  badSize.pop_back();
  std::vector<std::byte> unknownType = valid;
  unknownType[3] = std::byte{0x7F};

  client.socket.send(std::span<const std::byte>{}, client.server);  // empty
  client.socket.send("abc", client.server);                         // short
  client.socket.send(std::string(MAX_DATAGRAM_SIZE + 1, 'x'), client.server);
  client.socket.send(badMagic, client.server);
  client.socket.send(badSize, client.server);
  client.socket.send(unknownType, client.server);
  // A valid packet sent after the garbage proves the thread is still alive.
  client.send(Ping{.timestampMs = 42});

  const auto messages = receiveWithin(network);
  ASSERT_EQ(messages.size(), 1U);
  const auto* ping = std::get_if<Ping>(&messages[0].packet.payload);
  ASSERT_NE(ping, nullptr);
  EXPECT_EQ(ping->timestampMs, 42U);
}

TEST(ServerNetwork, RejectsConnectRequestOfAnotherVersion) {
  ServerNetwork network(0);
  TestClient client(network);

  std::vector<std::byte> request =
      encode(ConnectRequest{.playerName = "Old"}, 0);
  request[2] = std::byte{VERSION + 1};
  client.socket.send(request, client.server);

  std::optional<Packet> answer;
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(1);
  while (!answer && std::chrono::steady_clock::now() < deadline) {
    answer = client.poll();
  }
  ASSERT_TRUE(answer.has_value());
  const auto* reject = std::get_if<ConnectReject>(&answer->payload);
  ASSERT_NE(reject, nullptr);
  EXPECT_EQ(reject->reason, RejectReason::VersionMismatch);
  EXPECT_TRUE(network.receive().empty());
}

TEST(ServerNetwork, SendIncrementsTheSequence) {
  ServerNetwork network(0);
  TestClient client(network);
  const auto clientEndpoint = ampersand::net::UdpSocket::resolve(
      "127.0.0.1", client.socket.localPort());

  network.send(Pong{.timestampMs = 1}, clientEndpoint);
  network.send(Pong{.timestampMs = 2}, clientEndpoint);

  std::vector<Packet> packets;
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(1);
  while (packets.size() < 2 && std::chrono::steady_clock::now() < deadline) {
    if (auto packet = client.poll()) {
      packets.push_back(*packet);
    }
  }
  ASSERT_EQ(packets.size(), 2U);
  EXPECT_EQ(packets[1].header.sequence, packets[0].header.sequence + 1);
}
