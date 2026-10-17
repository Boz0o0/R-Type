#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <rtype/common/protocol/Packets.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <rtype/server/GameServer.hpp>
#include <rtype/server/ServerNetwork.hpp>
#include <string>
#include <thread>
#include <vector>

#include "ServerTestHelpers.hpp"

using namespace rtype::protocol;
using rtype::server::GameServer;
using rtype::server::ServerNetwork;
using rtype::server::test::stepUntil;
using rtype::server::test::TestClient;

namespace {

constexpr std::chrono::milliseconds NO_ANSWER_WAIT{200};

const GameServer::Clock::time_point START = GameServer::Clock::now();

std::optional<ConnectAccept> join(GameServer& game, TestClient& client,
                                  const std::string& name = "Player") {
  client.send(ConnectRequest{.playerName = name});
  return stepUntil<ConnectAccept>(game, client, START);
}

}  // namespace

TEST(GameServer, AcceptsAClientAndGivesItAnId) {
  ServerNetwork network(0);
  GameServer game(network);
  TestClient client(network);

  const auto accept = join(game, client, "Alice");

  ASSERT_TRUE(accept.has_value());
  EXPECT_EQ(accept->playerId, 0);
  EXPECT_EQ(accept->tickRate, SERVER_TICK_RATE);
  EXPECT_EQ(game.playerCount(), 1U);
}

TEST(GameServer, GivesEachClientItsOwnId) {
  ServerNetwork network(0);
  GameServer game(network);
  TestClient first(network);
  TestClient second(network);

  const auto firstAccept = join(game, first);
  const auto secondAccept = join(game, second);

  ASSERT_TRUE(firstAccept && secondAccept);
  EXPECT_EQ(firstAccept->playerId, 0);
  EXPECT_EQ(secondAccept->playerId, 1);
  EXPECT_EQ(game.playerCount(), 2U);
}

TEST(GameServer, SameAddressGetsTheSameIdAgain) {
  ServerNetwork network(0);
  GameServer game(network);
  TestClient client(network);

  const auto first = join(game, client);
  const auto again = join(game, client);

  ASSERT_TRUE(first && again);
  EXPECT_EQ(first->playerId, again->playerId);
  EXPECT_EQ(game.playerCount(), 1U);
}

TEST(GameServer, RejectsClientsWhenFull) {
  ServerNetwork network(0);
  GameServer game(network);
  std::vector<std::unique_ptr<TestClient>> players;
  for (std::uint8_t index = 0; index < MAX_PLAYERS; ++index) {
    players.push_back(std::make_unique<TestClient>(network));
    ASSERT_TRUE(join(game, *players.back()).has_value());
  }

  TestClient extra(network);
  extra.send(ConnectRequest{.playerName = "Late"});
  const auto reject = stepUntil<ConnectReject>(game, extra, START);

  ASSERT_TRUE(reject.has_value());
  EXPECT_EQ(reject->reason, RejectReason::ServerFull);
  EXPECT_EQ(game.playerCount(), MAX_PLAYERS);
}

TEST(GameServer, FreedIdIsGivenToTheNextClient) {
  ServerNetwork network(0);
  GameServer game(network);
  TestClient first(network);
  TestClient second(network);
  ASSERT_TRUE(join(game, first).has_value());
  ASSERT_TRUE(join(game, second).has_value());

  first.send(Disconnect{.reason = DisconnectReason::Quit});
  // The second client sends a PING so we know the DISCONNECT was handled.
  second.send(Ping{});
  ASSERT_TRUE(stepUntil<Pong>(game, second, START).has_value());
  EXPECT_EQ(game.playerCount(), 1U);

  TestClient third(network);
  const auto accept = join(game, third);
  ASSERT_TRUE(accept.has_value());
  EXPECT_EQ(accept->playerId, 0);
}

TEST(GameServer, AnswersPingWithPong) {
  ServerNetwork network(0);
  GameServer game(network);
  TestClient client(network);
  ASSERT_TRUE(join(game, client).has_value());

  client.send(Ping{.timestampMs = 1234});
  const auto pong = stepUntil<Pong>(game, client, START);

  ASSERT_TRUE(pong.has_value());
  EXPECT_EQ(pong->timestampMs, 1234U);
}

TEST(GameServer, IgnoresPacketsFromUnknownAddresses) {
  ServerNetwork network(0);
  GameServer game(network);
  TestClient stranger(network);

  stranger.send(Ping{.timestampMs = 1});
  stranger.send(PlayerInput{.clientTick = 1, .inputMask = InputFlag::SHOOT});

  EXPECT_FALSE(
      stepUntil<Pong>(game, stranger, START, NO_ANSWER_WAIT).has_value());
  EXPECT_EQ(game.playerCount(), 0U);
}

TEST(GameServer, SendsSnapshotsToClients) {
  ServerNetwork network(0);
  GameServer game(network);
  TestClient client(network);
  ASSERT_TRUE(join(game, client).has_value());

  const auto first = stepUntil<WorldSnapshot>(game, client, START);
  const auto second = stepUntil<WorldSnapshot>(game, client, START);

  ASSERT_TRUE(first && second);
  EXPECT_GT(second->serverTick, first->serverTick);
  EXPECT_EQ(first->serverTick % GameServer::SNAPSHOT_INTERVAL, 0U);
}

TEST(GameServer, RemovesSilentClientsAfterTheTimeout) {
  ServerNetwork network(0);
  GameServer game(network);
  TestClient client(network);
  ASSERT_TRUE(join(game, client).has_value());

  game.step(START + GameServer::CLIENT_TIMEOUT);
  EXPECT_EQ(game.playerCount(), 1U);

  game.step(START + GameServer::CLIENT_TIMEOUT + std::chrono::milliseconds(1));
  EXPECT_EQ(game.playerCount(), 0U);
}

TEST(GameServer, KeepsTickingWithoutAnyClient) {
  ServerNetwork network(0);
  GameServer game(network);
  std::atomic<bool> stopRequested{false};

  std::jthread gameThread([&game, &stopRequested] { game.run(stopRequested); });
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  stopRequested = true;
  gameThread.join();

  // 30 ticks in 500 ms at 60 Hz; leave room for a slow CI machine.
  EXPECT_GE(game.tick(), 15U);
  EXPECT_LE(game.tick(), 40U);
}

TEST(GameServer, TellsClientsWhenItShutsDown) {
  ServerNetwork network(0);
  GameServer game(network);
  TestClient client(network);
  ASSERT_TRUE(join(game, client).has_value());

  const std::atomic<bool> stopRequested{true};
  game.run(stopRequested);

  EXPECT_EQ(game.playerCount(), 0U);
  const auto disconnect =
      stepUntil<Disconnect>(game, client, START, NO_ANSWER_WAIT);
  ASSERT_TRUE(disconnect.has_value());
  EXPECT_EQ(disconnect->reason, DisconnectReason::ServerShutdown);
}
