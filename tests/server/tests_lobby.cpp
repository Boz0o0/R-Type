#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <rtype/common/protocol/Packets.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <rtype/server/Lobby.hpp>
#include <rtype/server/ServerNetwork.hpp>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#include "ServerTestHelpers.hpp"

using namespace rtype::protocol;
using rtype::server::Lobby;
using rtype::server::ServerNetwork;
using rtype::server::test::DEFAULT_WAIT;
using rtype::server::test::payloadsOf;
using rtype::server::test::TestClient;

namespace {

constexpr std::chrono::milliseconds NO_ANSWER_WAIT{200};

/**
 * @brief Runs steps of the lobby until the client receives a T, skipping the
 *        other packets.
 */
template <typename T>
std::optional<T> stepUntil(Lobby& lobby, TestClient& client,
                           std::chrono::milliseconds wait = DEFAULT_WAIT) {
  const auto deadline = std::chrono::steady_clock::now() + wait;
  while (std::chrono::steady_clock::now() < deadline) {
    lobby.step();
    while (const auto packet = client.poll()) {
      if (const auto* payload = std::get_if<T>(&packet->payload)) {
        return *payload;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return std::nullopt;
}

/** @brief Runs steps of the lobby until a condition holds. */
bool stepUntil(Lobby& lobby, const std::function<bool()>& condition) {
  const auto deadline = std::chrono::steady_clock::now() + DEFAULT_WAIT;
  while (std::chrono::steady_clock::now() < deadline) {
    lobby.step();
    if (condition()) {
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return false;
}

/** @brief Every packet the client receives for a while, the lobby running. */
std::vector<Packet> collect(Lobby& lobby, TestClient& client) {
  const auto deadline = std::chrono::steady_clock::now() + NO_ANSWER_WAIT;
  std::vector<Packet> packets;
  while (std::chrono::steady_clock::now() < deadline) {
    lobby.step();
    while (auto packet = client.poll()) {
      packets.push_back(std::move(*packet));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return packets;
}

std::optional<ConnectAccept> join(Lobby& lobby, TestClient& client,
                                  const std::string& name = "Player") {
  client.send(ConnectRequest{.playerName = name});
  return stepUntil<ConnectAccept>(lobby, client);
}

/** @brief Fills a party with MAX_PLAYERS clients. */
std::vector<std::unique_ptr<TestClient>> fillParty(Lobby& lobby,
                                                   ServerNetwork& network) {
  std::vector<std::unique_ptr<TestClient>> players;
  for (std::uint8_t index = 0; index < MAX_PLAYERS; ++index) {
    players.push_back(std::make_unique<TestClient>(network));
    EXPECT_TRUE(join(lobby, *players.back()).has_value());
  }
  return players;
}

}  // namespace

TEST(Lobby, StartsWithoutAnyParty) {
  ServerNetwork network(0);
  Lobby lobby(network);

  lobby.step();

  EXPECT_EQ(lobby.partyCount(), 0U);
  EXPECT_EQ(lobby.playerCount(), 0U);
}

TEST(Lobby, PutsTheFirstPlayersInTheSameParty) {
  ServerNetwork network(0);
  Lobby lobby(network);

  const auto players = fillParty(lobby, network);

  EXPECT_EQ(lobby.partyCount(), 1U);
  EXPECT_EQ(lobby.playerCount(), MAX_PLAYERS);
}

TEST(Lobby, OpensAnotherPartyWhenTheFirstIsFull) {
  ServerNetwork network(0);
  Lobby lobby(network);
  const auto players = fillParty(lobby, network);

  TestClient late(network);
  const auto accept = join(lobby, late, "Late");

  ASSERT_TRUE(accept.has_value());
  EXPECT_EQ(accept->playerId, 0);  // first player of its own party
  EXPECT_EQ(lobby.partyCount(), 2U);
  EXPECT_EQ(lobby.playerCount(), MAX_PLAYERS + 1U);
}

TEST(Lobby, PartiesDoNotSeeEachOther) {
  ServerNetwork network(0);
  Lobby lobby(network);
  const auto players = fillParty(lobby, network);
  collect(lobby, *players.front());  // what it got while the others joined

  TestClient late(network);
  ASSERT_TRUE(join(lobby, late, "Late").has_value());
  const auto latePackets = collect(lobby, late);
  const auto firstPackets = collect(lobby, *players.front());

  const auto lateJoins = payloadsOf<PlayerJoined>(latePackets);
  ASSERT_FALSE(lateJoins.empty());
  for (const PlayerJoined& joined : lateJoins) {
    EXPECT_EQ(joined.playerName, "Late");
  }
  // Its own party may send it again the joins it did not acknowledge.
  for (const PlayerJoined& joined : payloadsOf<PlayerJoined>(firstPackets)) {
    EXPECT_NE(joined.playerName, "Late");
  }
}

TEST(Lobby, ReusesAPlaceFreedInAParty) {
  ServerNetwork network(0);
  Lobby lobby(network);
  auto players = fillParty(lobby, network);

  players.back()->send(Disconnect{.reason = DisconnectReason::Quit});
  ASSERT_TRUE(stepUntil(
      lobby, [&lobby] { return lobby.playerCount() == MAX_PLAYERS - 1U; }));
  TestClient next(network);
  const auto accept = join(lobby, next);

  ASSERT_TRUE(accept.has_value());
  EXPECT_EQ(accept->playerId, MAX_PLAYERS - 1);
  EXPECT_EQ(lobby.partyCount(), 1U);
}

TEST(Lobby, ClosesAPartyOnceEveryoneLeft) {
  ServerNetwork network(0);
  Lobby lobby(network);
  TestClient client(network);
  ASSERT_TRUE(join(lobby, client).has_value());
  ASSERT_EQ(lobby.partyCount(), 1U);

  client.send(Disconnect{.reason = DisconnectReason::Quit});

  EXPECT_TRUE(stepUntil(lobby, [&lobby] { return lobby.partyCount() == 0; }));
  EXPECT_EQ(lobby.playerCount(), 0U);
}

TEST(Lobby, KeepsTheSamePartyForTheSameAddress) {
  ServerNetwork network(0);
  Lobby lobby(network);
  TestClient client(network);

  const auto first = join(lobby, client);
  const auto again = join(lobby, client);

  ASSERT_TRUE(first && again);
  EXPECT_EQ(first->playerId, again->playerId);
  EXPECT_EQ(lobby.partyCount(), 1U);
  EXPECT_EQ(lobby.playerCount(), 1U);
}

TEST(Lobby, RejectsBadNamesWithoutOpeningAParty) {
  ServerNetwork network(0);
  Lobby lobby(network);
  TestClient client(network);

  client.send(ConnectRequest{.playerName = "Bad\xFF\xFEName"});
  const auto reject = stepUntil<ConnectReject>(lobby, client);

  ASSERT_TRUE(reject.has_value());
  EXPECT_EQ(reject->reason, RejectReason::InvalidName);
  EXPECT_EQ(lobby.partyCount(), 0U);
}

TEST(Lobby, RejectsClientsWhenEveryPartyIsFull) {
  ServerNetwork network(0);
  Lobby lobby(network, 1);
  const auto players = fillParty(lobby, network);

  TestClient extra(network);
  extra.send(ConnectRequest{.playerName = "Late"});
  const auto reject = stepUntil<ConnectReject>(lobby, extra);

  ASSERT_TRUE(reject.has_value());
  EXPECT_EQ(reject->reason, RejectReason::ServerFull);
  EXPECT_EQ(lobby.partyCount(), 1U);
}

TEST(Lobby, IgnoresPacketsFromUnknownAddresses) {
  ServerNetwork network(0);
  Lobby lobby(network);
  TestClient stranger(network);

  stranger.send(Ping{.timestampMs = 1});

  EXPECT_FALSE(stepUntil<Pong>(lobby, stranger, NO_ANSWER_WAIT).has_value());
  EXPECT_EQ(lobby.partyCount(), 0U);
}

TEST(Lobby, PartiesTickOnTheirOwn) {
  ServerNetwork network(0);
  Lobby lobby(network);
  TestClient client(network);
  ASSERT_TRUE(join(lobby, client).has_value());

  // No step(): the party sends its snapshots from its own thread.
  std::optional<WorldSnapshot> snapshot;
  const auto deadline = std::chrono::steady_clock::now() + DEFAULT_WAIT;
  while (!snapshot && std::chrono::steady_clock::now() < deadline) {
    while (const auto packet = client.poll()) {
      if (const auto* payload = std::get_if<WorldSnapshot>(&packet->payload)) {
        snapshot = *payload;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  EXPECT_TRUE(snapshot.has_value());
}

TEST(Lobby, StopsEveryPartyWhenItShutsDown) {
  ServerNetwork network(0);
  Lobby lobby(network);
  const auto players = fillParty(lobby, network);
  TestClient late(network);
  ASSERT_TRUE(join(lobby, late).has_value());
  ASSERT_EQ(lobby.partyCount(), 2U);

  const std::atomic<bool> stopRequested{true};
  lobby.run(stopRequested);

  EXPECT_EQ(lobby.partyCount(), 0U);
  for (TestClient* client : {players.front().get(), &late}) {
    std::optional<Disconnect> disconnect;
    const auto deadline = std::chrono::steady_clock::now() + DEFAULT_WAIT;
    while (!disconnect && std::chrono::steady_clock::now() < deadline) {
      if (const auto packet = client->poll()) {
        if (const auto* payload = std::get_if<Disconnect>(&packet->payload)) {
          disconnect = *payload;
        }
      }
    }
    ASSERT_TRUE(disconnect.has_value());
    EXPECT_EQ(disconnect->reason, DisconnectReason::ServerShutdown);
  }
}
