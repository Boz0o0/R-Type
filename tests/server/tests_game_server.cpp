#include <gtest/gtest.h>

#include <ampersand/physics/components/Velocity.hpp>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <rtype/common/components/Kind.hpp>
#include <rtype/common/protocol/Packets.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <rtype/server/Game.hpp>
#include <rtype/server/GameServer.hpp>
#include <rtype/server/ReliableOutbox.hpp>
#include <rtype/server/ServerNetwork.hpp>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#include "ServerTestHelpers.hpp"

using namespace rtype::protocol;
using rtype::server::GameServer;
using rtype::server::ServerNetwork;
using rtype::server::test::collect;
using rtype::server::test::payloadsOf;
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

/** @brief Owners of the ships spawned in a list of packets, in order. */
std::vector<std::uint8_t> shipOwners(const std::vector<Packet>& packets) {
  std::vector<std::uint8_t> owners;
  for (const EntitySpawn& spawn : payloadsOf<EntitySpawn>(packets)) {
    if (spawn.entityType == rtype::common::EntityKind::Ship) {
      owners.push_back(spawn.ownerId);
    }
  }
  return owners;
}

/** @brief Entity id of the ship of a player in a list of packets, or 0. */
std::uint32_t shipIdOf(const std::vector<Packet>& packets,
                       std::uint8_t playerId) {
  for (const EntitySpawn& spawn : payloadsOf<EntitySpawn>(packets)) {
    if (spawn.entityType == rtype::common::EntityKind::Ship &&
        spawn.ownerId == playerId) {
      return spawn.entityId;
    }
  }
  return 0;
}

/** @brief Header sequences of the packets holding a T. */
template <typename T>
std::vector<std::uint32_t> sequencesOf(const std::vector<Packet>& packets) {
  std::vector<std::uint32_t> sequences;
  for (const Packet& packet : packets) {
    if (std::holds_alternative<T>(packet.payload)) {
      sequences.push_back(packet.header.sequence);
    }
  }
  return sequences;
}

/** @brief Sends an ACK for every reliable packet of the list. */
void acknowledge(TestClient& client, const std::vector<Packet>& packets) {
  for (const Packet& packet : packets) {
    if (packet.header.isReliable()) {
      client.send(Ack{.sequence = packet.header.sequence});
    }
  }
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

TEST(GameServer, RejectsNamesThatAreNotUtf8) {
  ServerNetwork network(0);
  GameServer game(network);
  TestClient client(network);

  client.send(ConnectRequest{.playerName = "Bad\xFF\xFEName"});
  const auto reject = stepUntil<ConnectReject>(game, client, START);

  ASSERT_TRUE(reject.has_value());
  EXPECT_EQ(reject->reason, RejectReason::InvalidName);
  EXPECT_EQ(game.playerCount(), 0U);
}

TEST(GameServer, WelcomesANewPlayerWithTheGame) {
  ServerNetwork network(0);
  GameServer game(network);
  TestClient client(network);
  ASSERT_TRUE(join(game, client, "Alice").has_value());

  const auto packets = collect(game, client, START);

  const auto joined = payloadsOf<PlayerJoined>(packets);
  ASSERT_EQ(joined.size(), 1U);
  EXPECT_EQ(joined[0].playerId, 0);
  EXPECT_EQ(joined[0].playerName, "Alice");
  EXPECT_EQ(payloadsOf<GameStart>(packets).size(), 1U);
  const auto spawns = payloadsOf<EntitySpawn>(packets);
  ASSERT_FALSE(spawns.empty());
  EXPECT_EQ(spawns[0].entityType, rtype::common::EntityKind::Ship);
  EXPECT_EQ(spawns[0].ownerId, 0);
  const auto states = payloadsOf<PlayerState>(packets);
  ASSERT_FALSE(states.empty());
  EXPECT_EQ(states[0].lives, rtype::server::Game::STARTING_LIVES);
}

TEST(GameServer, ShowsTheOthersToALatePlayer) {
  ServerNetwork network(0);
  GameServer game(network);
  TestClient first(network);
  TestClient second(network);
  ASSERT_TRUE(join(game, first, "Alice").has_value());
  (void)collect(game, first, START);  // the ship of Alice is announced

  ASSERT_TRUE(join(game, second, "Bob").has_value());
  const auto toBob = collect(game, second, START);
  const auto toAlice = collect(game, first, START);

  EXPECT_EQ(payloadsOf<PlayerJoined>(toBob).size(), 2U);
  EXPECT_EQ(payloadsOf<GameStart>(toBob).size(), 1U);
  EXPECT_EQ(shipOwners(toBob), (std::vector<std::uint8_t>{0, 1}));
  const auto bobForAlice = payloadsOf<PlayerJoined>(toAlice);
  ASSERT_EQ(bobForAlice.size(), 1U);
  EXPECT_EQ(bobForAlice[0].playerName, "Bob");
}

TEST(GameServer, TellsTheOthersWhenAPlayerQuits) {
  ServerNetwork network(0);
  GameServer game(network);
  TestClient first(network);
  TestClient second(network);
  ASSERT_TRUE(join(game, first).has_value());
  ASSERT_TRUE(join(game, second).has_value());
  const std::uint32_t firstShip = shipIdOf(collect(game, second, START), 0);
  ASSERT_NE(firstShip, 0U);

  first.send(Disconnect{.reason = DisconnectReason::Quit});
  const auto left = stepUntil<PlayerLeft>(game, second, START);
  const auto destroyed = stepUntil<EntityDestroy>(game, second, START);

  ASSERT_TRUE(left.has_value());
  EXPECT_EQ(left->playerId, 0);
  EXPECT_EQ(left->reason, DisconnectReason::Quit);
  ASSERT_TRUE(destroyed.has_value());
  EXPECT_EQ(destroyed->entityId, firstShip);
}

TEST(GameServer, TellsTheOthersWhenAPlayerCrashes) {
  ServerNetwork network(0);
  GameServer game(network);
  TestClient crashed(network);
  TestClient alive(network);
  ASSERT_TRUE(join(game, crashed).has_value());
  ASSERT_TRUE(join(game, alive).has_value());

  // Only the second client keeps talking.
  const auto later = START + std::chrono::seconds(4);
  alive.send(Ping{});
  ASSERT_TRUE(stepUntil<Pong>(game, alive, later).has_value());
  const auto left = stepUntil<PlayerLeft>(
      game, alive,
      START + GameServer::CLIENT_TIMEOUT + std::chrono::milliseconds(1));

  ASSERT_TRUE(left.has_value());
  EXPECT_EQ(left->playerId, 0);
  EXPECT_EQ(left->reason, DisconnectReason::Timeout);
  EXPECT_EQ(game.playerCount(), 1U);
}

TEST(GameServer, SnapshotsCarryTheEntities) {
  ServerNetwork network(0);
  GameServer game(network);
  TestClient client(network);
  ASSERT_TRUE(join(game, client).has_value());

  const auto packets = collect(game, client, START);

  const auto spawns = payloadsOf<EntitySpawn>(packets);
  ASSERT_FALSE(spawns.empty());
  const auto snapshots = payloadsOf<WorldSnapshot>(packets);
  ASSERT_FALSE(snapshots.empty());
  ASSERT_FALSE(snapshots.back().entities.empty());
  EXPECT_EQ(snapshots.back().entities[0].entityId, spawns[0].entityId);
}

TEST(GameServer, ResendsReliablePacketsUntilAcknowledged) {
  ServerNetwork network(0);
  GameServer game(network);
  TestClient client(network);
  client.send(ConnectRequest{.playerName = "Alice"});
  const auto sent = sequencesOf<ConnectAccept>(collect(game, client, START));
  ASSERT_EQ(sent.size(), 1U);

  const auto resent = collect(
      game, client, START + rtype::server::ReliableOutbox::RESEND_INTERVAL);
  EXPECT_EQ(sequencesOf<ConnectAccept>(resent), sent);  // same sequence
  acknowledge(client, resent);
  // The PONG comes after the ACKs, so they are handled.
  client.send(Ping{});
  ASSERT_TRUE(
      stepUntil<Pong>(game, client,
                      START + rtype::server::ReliableOutbox::RESEND_INTERVAL)
          .has_value());

  const auto afterAck =
      collect(game, client,
              START + (rtype::server::ReliableOutbox::RESEND_INTERVAL * 3));
  EXPECT_TRUE(payloadsOf<ConnectAccept>(afterAck).empty());
}

TEST(GameServer, DropsAClientThatNeverAcknowledges) {
  ServerNetwork network(0);
  GameServer game(network);
  TestClient client(network);
  ASSERT_TRUE(join(game, client).has_value());

  auto now = START;
  for (std::uint32_t attempt = 0; attempt <= MAX_SEND_ATTEMPTS; ++attempt) {
    now += rtype::server::ReliableOutbox::RESEND_INTERVAL;
    game.step(now);
  }

  EXPECT_EQ(game.playerCount(), 0U);
}

TEST(GameServer, MovesTheShipWithTheInputs) {
  ServerNetwork network(0);
  GameServer game(network);
  TestClient client(network);
  ASSERT_TRUE(join(game, client).has_value());
  game.step(START);
  const auto ship = game.game().shipOf(0);
  ASSERT_TRUE(ship.has_value());
  auto& world = game.game().world();

  client.send(PlayerInput{.clientTick = 5, .inputMask = InputFlag::MOVE_RIGHT});
  // Older than the previous one: arrived late, ignored.
  client.send(PlayerInput{.clientTick = 4, .inputMask = InputFlag::MOVE_LEFT});
  client.send(Ping{});
  ASSERT_TRUE(stepUntil<Pong>(game, client, START).has_value());
  game.step(START);

  EXPECT_GT(world.get<ampersand::physics::Velocity>(*ship).value.x, 0.0F);
}
