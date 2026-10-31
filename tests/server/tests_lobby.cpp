#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <rtype/common/protocol/PacketCodec.hpp>
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
 * @brief Runs steps of the lobby at a given time until the client receives a
 *        T, skipping the other packets.
 */
template <typename T>
std::optional<T> stepUntilAt(Lobby& lobby, TestClient& client,
                             Lobby::Clock::time_point now,
                             std::chrono::milliseconds wait = DEFAULT_WAIT) {
  const auto deadline = std::chrono::steady_clock::now() + wait;
  while (std::chrono::steady_clock::now() < deadline) {
    lobby.step(now);
    while (const auto packet = client.poll()) {
      if (const auto* payload = std::get_if<T>(&packet->payload)) {
        return *payload;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return std::nullopt;
}

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

/** @brief Sends a request and waits for the answer of type T. */
template <typename T>
std::optional<T> ask(Lobby& lobby, TestClient& client, const Payload& request) {
  client.send(request);
  return stepUntil<T>(lobby, client);
}

/** @brief Enters the lobby. */
std::optional<ConnectAccept> connect(Lobby& lobby, TestClient& client,
                                     const std::string& name = "Player") {
  return ask<ConnectAccept>(lobby, client, ConnectRequest{.playerName = name});
}

/** @brief Enters the lobby, then joins a room with ROOM_JOIN. */
std::optional<RoomJoined> join(Lobby& lobby, TestClient& client,
                               const std::string& name = "Player",
                               const RoomJoin& request = {}) {
  if (!connect(lobby, client, name)) {
    return std::nullopt;
  }
  return ask<RoomJoined>(lobby, client, request);
}

/** @brief Enters the lobby, then joins the private room of a code. */
std::optional<RoomJoined> joinCode(Lobby& lobby, TestClient& client,
                                   const std::string& name,
                                   const std::string& code) {
  return join(lobby, client, name, RoomJoin{.roomId = 0, .code = code});
}

/** @brief Enters the lobby, then opens a room with ROOM_CREATE. */
std::optional<RoomJoined> create(
    Lobby& lobby, TestClient& client, const std::string& name = "Player",
    RoomVisibility visibility = RoomVisibility::Public) {
  if (!connect(lobby, client, name)) {
    return std::nullopt;
  }
  return ask<RoomJoined>(lobby, client, RoomCreate{.visibility = visibility});
}

/** @brief Fills a room with MAX_PLAYERS clients. */
std::vector<std::unique_ptr<TestClient>> fillParty(
    Lobby& lobby, ServerNetwork& network, const RoomJoin& request = {}) {
  std::vector<std::unique_ptr<TestClient>> players;
  for (std::uint8_t index = 0; index < MAX_PLAYERS; ++index) {
    players.push_back(std::make_unique<TestClient>(network));
    EXPECT_TRUE(join(lobby, *players.back(), "Player", request).has_value());
  }
  return players;
}

}  // namespace

// The lobby

TEST(Lobby, StartsWithoutAnyParty) {
  ServerNetwork network(0);
  Lobby lobby(network);

  lobby.step();

  EXPECT_EQ(lobby.partyCount(), 0U);
  EXPECT_EQ(lobby.playerCount(), 0U);
  EXPECT_EQ(lobby.visitorCount(), 0U);
}

TEST(Lobby, PutsANewClientInTheLobby) {
  ServerNetwork network(0);
  Lobby lobby(network);
  TestClient client(network);

  const auto accept = connect(lobby, client, "Alice");

  ASSERT_TRUE(accept.has_value());
  EXPECT_EQ(accept->tickRate, SERVER_TICK_RATE);
  EXPECT_EQ(lobby.visitorCount(), 1U);
  EXPECT_EQ(lobby.partyCount(), 0U);
  EXPECT_FALSE(lobby.partyOf(client.endpoint()).has_value());
}

TEST(Lobby, AnswersARepeatedConnectRequest) {
  ServerNetwork network(0);
  Lobby lobby(network);
  TestClient client(network);
  ASSERT_TRUE(connect(lobby, client).has_value());

  // The client did not get CONNECT_ACCEPT in time and asks again.
  EXPECT_TRUE(connect(lobby, client).has_value());
  EXPECT_EQ(lobby.visitorCount(), 1U);
}

TEST(Lobby, RejectsBadNames) {
  ServerNetwork network(0);
  Lobby lobby(network);
  TestClient client(network);

  const auto reject = ask<ConnectReject>(
      lobby, client, ConnectRequest{.playerName = "Bad\xFF\xFEName"});

  ASSERT_TRUE(reject.has_value());
  EXPECT_EQ(reject->reason, RejectReason::InvalidName);
  EXPECT_EQ(lobby.visitorCount(), 0U);
}

TEST(Lobby, IgnoresPacketsFromUnknownAddresses) {
  ServerNetwork network(0);
  Lobby lobby(network);
  TestClient stranger(network);

  stranger.send(Ping{.timestampMs = 1});
  stranger.send(RoomList{});
  stranger.send(RoomCreate{});

  const auto packets = collect(lobby, stranger);
  EXPECT_TRUE(packets.empty());
  EXPECT_EQ(lobby.partyCount(), 0U);
  EXPECT_EQ(lobby.visitorCount(), 0U);
}

TEST(Lobby, AnswersPingsInTheLobby) {
  ServerNetwork network(0);
  Lobby lobby(network);
  TestClient client(network);
  ASSERT_TRUE(connect(lobby, client).has_value());

  const auto pong = ask<Pong>(lobby, client, Ping{.timestampMs = 1234});

  ASSERT_TRUE(pong.has_value());
  EXPECT_EQ(pong->timestampMs, 1234U);
}

TEST(Lobby, DropsSilentClientsOfTheLobbyOnly) {
  ServerNetwork network(0);
  Lobby lobby(network);
  TestClient visitor(network);
  TestClient player(network);
  ASSERT_TRUE(connect(lobby, visitor).has_value());
  ASSERT_TRUE(join(lobby, player).has_value());
  const auto now = Lobby::Clock::now();

  lobby.step(now + Lobby::CLIENT_TIMEOUT + std::chrono::milliseconds(100));

  EXPECT_EQ(lobby.visitorCount(), 0U);
  // Its party times it out, not the lobby.
  EXPECT_TRUE(lobby.partyOf(player.endpoint()).has_value());
}

TEST(Lobby, ForgetsAClientThatQuitsTheLobby) {
  ServerNetwork network(0);
  Lobby lobby(network);
  TestClient client(network);
  ASSERT_TRUE(connect(lobby, client).has_value());

  client.send(Disconnect{.reason = DisconnectReason::Quit});

  EXPECT_TRUE(stepUntil(lobby, [&lobby] { return lobby.visitorCount() == 0; }));
}

// ROOM_LIST

TEST(Lobby, ListsNoRoomOnAnEmptyServer) {
  ServerNetwork network(0);
  Lobby lobby(network);
  TestClient client(network);
  ASSERT_TRUE(connect(lobby, client).has_value());

  const auto list = ask<RoomListReply>(lobby, client, RoomList{});

  ASSERT_TRUE(list.has_value());
  EXPECT_TRUE(list->rooms.empty());
}

TEST(Lobby, ListsThePublicRoomsOnly) {
  ServerNetwork network(0);
  Lobby lobby(network);
  TestClient alice(network);
  TestClient bob(network);
  TestClient carol(network);
  const auto publicRoom = join(lobby, alice, "Alice");
  ASSERT_TRUE(publicRoom.has_value());
  ASSERT_TRUE(joinCode(lobby, bob, "Bob", "SECRET").has_value());
  ASSERT_TRUE(connect(lobby, carol, "Carol").has_value());

  const auto list = ask<RoomListReply>(lobby, carol, RoomList{});

  ASSERT_TRUE(list.has_value());
  ASSERT_EQ(list->rooms.size(), 1U);
  EXPECT_EQ(list->rooms[0].roomId, publicRoom->roomId);
  EXPECT_EQ(list->rooms[0].players, 1);
}

TEST(Lobby, SendsTheRoomListAtMostOncePerCooldown) {
  ServerNetwork network(0);
  Lobby lobby(network);
  TestClient client(network);
  ASSERT_TRUE(connect(lobby, client).has_value());
  const auto now = Lobby::Clock::now();
  client.send(RoomList{});
  ASSERT_TRUE(stepUntilAt<RoomListReply>(lobby, client, now).has_value());

  client.send(RoomList{});
  EXPECT_FALSE(stepUntilAt<RoomListReply>(lobby, client, now, NO_ANSWER_WAIT)
                   .has_value());

  client.send(RoomList{});
  EXPECT_TRUE(
      stepUntilAt<RoomListReply>(lobby, client, now + Lobby::ROOM_LIST_COOLDOWN)
          .has_value());
}

// ROOM_CREATE

TEST(Lobby, CreatesAPublicRoom) {
  ServerNetwork network(0);
  Lobby lobby(network);
  TestClient alice(network);
  TestClient bob(network);

  const auto created = create(lobby, alice, "Alice");
  const auto joined = join(lobby, bob, "Bob");  // quick match

  ASSERT_TRUE(created && joined);
  EXPECT_EQ(created->playerId, 0);
  EXPECT_TRUE(created->code.empty());
  EXPECT_EQ(joined->roomId, created->roomId);
  EXPECT_EQ(joined->playerId, 1);
  EXPECT_EQ(lobby.partyCount(), 1U);
}

TEST(Lobby, AlwaysOpensANewRoomOnCreate) {
  ServerNetwork network(0);
  Lobby lobby(network);
  TestClient alice(network);
  TestClient bob(network);

  const auto first = create(lobby, alice, "Alice");
  const auto second = create(lobby, bob, "Bob");

  ASSERT_TRUE(first && second);
  EXPECT_NE(first->roomId, second->roomId);
  EXPECT_EQ(second->playerId, 0);
  EXPECT_EQ(lobby.partyCount(), 2U);
}

TEST(Lobby, GivesAPrivateRoomACodeOfDigits) {
  ServerNetwork network(0);
  Lobby lobby(network);
  TestClient alice(network);
  TestClient bob(network);
  TestClient carol(network);

  const auto created = create(lobby, alice, "Alice", RoomVisibility::Private);
  ASSERT_TRUE(created.has_value());
  const auto joined = joinCode(lobby, bob, "Bob", created->code);
  ASSERT_TRUE(connect(lobby, carol, "Carol").has_value());
  const auto list = ask<RoomListReply>(lobby, carol, RoomList{});

  ASSERT_EQ(created->code.size(), Lobby::GENERATED_CODE_LENGTH);
  for (const char digit : created->code) {
    EXPECT_TRUE(digit >= '0' && digit <= '9') << created->code;
  }
  EXPECT_EQ(lobby.partyWithCode(created->code), created->roomId);
  ASSERT_TRUE(joined.has_value());
  EXPECT_EQ(joined->roomId, created->roomId);
  EXPECT_EQ(joined->code, created->code);
  ASSERT_TRUE(list.has_value());
  EXPECT_TRUE(list->rooms.empty());
}

TEST(Lobby, RefusesToCreateRoomsPastTheLimit) {
  ServerNetwork network(0);
  Lobby lobby(network, 1);
  TestClient first(network);
  TestClient second(network);
  ASSERT_TRUE(create(lobby, first).has_value());
  ASSERT_TRUE(connect(lobby, second).has_value());

  const auto reject = ask<RoomReject>(lobby, second, RoomCreate{});

  ASSERT_TRUE(reject.has_value());
  EXPECT_EQ(reject->reason, RejectReason::ServerFull);
  EXPECT_EQ(lobby.partyCount(), 1U);
  EXPECT_EQ(lobby.visitorCount(), 1U);
}

// ROOM_JOIN by id

TEST(Lobby, JoinsAPublicRoomById) {
  ServerNetwork network(0);
  Lobby lobby(network);
  TestClient alice(network);
  TestClient bob(network);
  TestClient carol(network);
  const auto first = create(lobby, alice, "Alice");
  ASSERT_TRUE(create(lobby, bob, "Bob").has_value());
  ASSERT_TRUE(first.has_value());

  const auto joined =
      join(lobby, carol, "Carol", RoomJoin{.roomId = first->roomId});

  ASSERT_TRUE(joined.has_value());
  EXPECT_EQ(joined->roomId, first->roomId);
  EXPECT_EQ(joined->playerId, 1);
}

TEST(Lobby, RefusesRoomIdsThatAreNotPublicRooms) {
  ServerNetwork network(0);
  Lobby lobby(network);
  TestClient alice(network);
  TestClient bob(network);
  const auto secret = create(lobby, alice, "Alice", RoomVisibility::Private);
  ASSERT_TRUE(secret.has_value());
  ASSERT_TRUE(connect(lobby, bob, "Bob").has_value());

  const auto unknown = ask<RoomReject>(lobby, bob, RoomJoin{.roomId = 42});
  const auto hidden =
      ask<RoomReject>(lobby, bob, RoomJoin{.roomId = secret->roomId});

  ASSERT_TRUE(unknown && hidden);
  EXPECT_EQ(unknown->reason, RejectReason::RoomNotFound);
  EXPECT_EQ(hidden->reason, RejectReason::RoomNotFound);
  EXPECT_FALSE(lobby.partyOf(bob.endpoint()).has_value());
}

TEST(Lobby, RefusesAFullRoom) {
  ServerNetwork network(0);
  Lobby lobby(network);
  const auto players = fillParty(lobby, network);
  const auto room = lobby.partyOf(players.front()->endpoint());
  ASSERT_TRUE(room.has_value());

  TestClient late(network);
  ASSERT_TRUE(connect(lobby, late, "Late").has_value());
  const auto reject = ask<RoomReject>(lobby, late, RoomJoin{.roomId = *room});

  ASSERT_TRUE(reject.has_value());
  EXPECT_EQ(reject->reason, RejectReason::PartyFull);
}

// Quick match (ROOM_JOIN without id nor code)

TEST(Lobby, PutsTheFirstPlayersInTheSameParty) {
  ServerNetwork network(0);
  Lobby lobby(network);

  const auto players = fillParty(lobby, network);

  EXPECT_EQ(lobby.partyCount(), 1U);
  EXPECT_EQ(lobby.playerCount(), MAX_PLAYERS);
  EXPECT_EQ(lobby.visitorCount(), 0U);
}

TEST(Lobby, OpensAnotherPartyWhenTheFirstIsFull) {
  ServerNetwork network(0);
  Lobby lobby(network);
  const auto players = fillParty(lobby, network);

  TestClient late(network);
  const auto joined = join(lobby, late, "Late");

  ASSERT_TRUE(joined.has_value());
  EXPECT_EQ(joined->playerId, 0);  // first player of its own party
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
  const auto joined = join(lobby, next);

  ASSERT_TRUE(joined.has_value());
  EXPECT_EQ(joined->playerId, MAX_PLAYERS - 1);
  EXPECT_EQ(lobby.partyCount(), 1U);
}

TEST(Lobby, RejectsClientsWhenEveryPartyIsFull) {
  ServerNetwork network(0);
  Lobby lobby(network, 1);
  const auto players = fillParty(lobby, network);

  TestClient extra(network);
  ASSERT_TRUE(connect(lobby, extra, "Late").has_value());
  const auto reject = ask<RoomReject>(lobby, extra, RoomJoin{});

  ASSERT_TRUE(reject.has_value());
  EXPECT_EQ(reject->reason, RejectReason::ServerFull);
  EXPECT_EQ(lobby.partyCount(), 1U);
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

// ROOM_JOIN with a code

TEST(Lobby, PutsPlayersWithTheSameCodeInTheSameParty) {
  ServerNetwork network(0);
  Lobby lobby(network);
  TestClient alice(network);
  TestClient bob(network);

  const auto first = joinCode(lobby, alice, "Alice", "friends");
  const auto joined = joinCode(lobby, bob, "Bob", "FRIENDS");  // case ignored

  ASSERT_TRUE(first && joined);
  EXPECT_EQ(joined->playerId, 1);
  EXPECT_EQ(joined->code, "FRIENDS");
  EXPECT_EQ(lobby.partyCount(), 1U);
  EXPECT_EQ(lobby.partyWithCode("Friends"), lobby.partyOf(alice.endpoint()));
  EXPECT_EQ(lobby.partyOf(alice.endpoint()), lobby.partyOf(bob.endpoint()));
}

TEST(Lobby, GivesEachCodeItsOwnParty) {
  ServerNetwork network(0);
  Lobby lobby(network);
  TestClient alice(network);
  TestClient bob(network);

  ASSERT_TRUE(joinCode(lobby, alice, "Alice", "ONE").has_value());
  const auto joined = joinCode(lobby, bob, "Bob", "TWO");

  ASSERT_TRUE(joined.has_value());
  EXPECT_EQ(joined->playerId, 0);
  EXPECT_EQ(lobby.partyCount(), 2U);
}

TEST(Lobby, NeverPutsPlayersWithoutCodeInAPrivateParty) {
  ServerNetwork network(0);
  Lobby lobby(network);
  TestClient friendly(network);
  TestClient stranger(network);
  ASSERT_TRUE(joinCode(lobby, friendly, "Friend", "SECRET").has_value());

  const auto joined = join(lobby, stranger, "Stranger");

  ASSERT_TRUE(joined.has_value());
  EXPECT_EQ(joined->playerId, 0);
  EXPECT_EQ(lobby.partyCount(), 2U);
  EXPECT_NE(lobby.partyOf(friendly.endpoint()),
            lobby.partyOf(stranger.endpoint()));
}

TEST(Lobby, RejectsPlayersWhenTheirPrivatePartyIsFull) {
  ServerNetwork network(0);
  Lobby lobby(network);
  const auto players =
      fillParty(lobby, network, RoomJoin{.roomId = 0, .code = "FULL"});

  TestClient late(network);
  ASSERT_TRUE(connect(lobby, late, "Late").has_value());
  const auto reject =
      ask<RoomReject>(lobby, late, RoomJoin{.roomId = 0, .code = "full"});

  ASSERT_TRUE(reject.has_value());
  EXPECT_EQ(reject->reason, RejectReason::PartyFull);
  EXPECT_EQ(lobby.partyCount(), 1U);
}

TEST(Lobby, RejectsInvalidPartyCodesWithoutOpeningAParty) {
  ServerNetwork network(0);
  Lobby lobby(network);
  TestClient client(network);
  ASSERT_TRUE(connect(lobby, client, "Bob").has_value());

  const auto reject =
      ask<RoomReject>(lobby, client, RoomJoin{.roomId = 0, .code = "ab-c"});

  ASSERT_TRUE(reject.has_value());
  EXPECT_EQ(reject->reason, RejectReason::InvalidPartyCode);
  EXPECT_EQ(lobby.partyCount(), 0U);
}

TEST(Lobby, RejectsNewCodesWhenItRunsTooManyParties) {
  ServerNetwork network(0);
  Lobby lobby(network, 1);
  TestClient first(network);
  ASSERT_TRUE(join(lobby, first).has_value());

  TestClient other(network);
  ASSERT_TRUE(connect(lobby, other, "Other").has_value());
  const auto reject =
      ask<RoomReject>(lobby, other, RoomJoin{.roomId = 0, .code = "NEW"});

  ASSERT_TRUE(reject.has_value());
  EXPECT_EQ(reject->reason, RejectReason::ServerFull);
  EXPECT_EQ(lobby.partyCount(), 1U);
}

TEST(Lobby, FreesTheCodeOnceItsPartyIsClosed) {
  ServerNetwork network(0);
  Lobby lobby(network);
  TestClient client(network);
  ASSERT_TRUE(joinCode(lobby, client, "Bob", "ONCE").has_value());
  ASSERT_TRUE(lobby.partyWithCode("ONCE").has_value());

  client.send(Disconnect{.reason = DisconnectReason::Quit});

  EXPECT_TRUE(stepUntil(lobby, [&lobby] { return lobby.partyCount() == 0; }));
  EXPECT_FALSE(lobby.partyWithCode("ONCE").has_value());
}

// ROOM_LEAVE and leaving the server

TEST(Lobby, BringsAPlayerWhoLeavesItsRoomBackToTheLobby) {
  ServerNetwork network(0);
  Lobby lobby(network);
  TestClient alice(network);
  TestClient bob(network);
  const auto room = create(lobby, alice, "Alice");
  ASSERT_TRUE(room.has_value());
  ASSERT_TRUE(
      join(lobby, bob, "Bob", RoomJoin{.roomId = room->roomId}).has_value());

  const auto left = ask<RoomLeft>(lobby, alice, RoomLeave{});
  const auto told = stepUntil<PlayerLeft>(lobby, bob);

  ASSERT_TRUE(left.has_value());
  ASSERT_TRUE(told.has_value());
  EXPECT_EQ(told->playerId, 0);
  EXPECT_FALSE(lobby.partyOf(alice.endpoint()).has_value());
  EXPECT_EQ(lobby.visitorCount(), 1U);
  EXPECT_EQ(lobby.playerCount(), 1U);
  // Back in the lobby: the Lobby answers it again.
  const auto list = ask<RoomListReply>(lobby, alice, RoomList{});
  ASSERT_TRUE(list.has_value());
  ASSERT_EQ(list->rooms.size(), 1U);
  EXPECT_EQ(list->rooms[0].players, 1);
}

TEST(Lobby, ClosesARoomWhenItsLastPlayerLeavesIt) {
  ServerNetwork network(0);
  Lobby lobby(network);
  TestClient client(network);
  ASSERT_TRUE(create(lobby, client).has_value());

  ASSERT_TRUE(ask<RoomLeft>(lobby, client, RoomLeave{}).has_value());

  EXPECT_TRUE(stepUntil(lobby, [&lobby] { return lobby.partyCount() == 0; }));
  EXPECT_EQ(lobby.visitorCount(), 1U);
}

TEST(Lobby, LetsAPlayerJoinAnotherRoomAfterLeaving) {
  ServerNetwork network(0);
  Lobby lobby(network);
  TestClient alice(network);
  TestClient bob(network);
  const auto first = create(lobby, alice, "Alice");
  ASSERT_TRUE(first.has_value());
  ASSERT_TRUE(create(lobby, bob, "Bob").has_value());
  ASSERT_TRUE(ask<RoomLeft>(lobby, alice, RoomLeave{}).has_value());

  const auto joined = ask<RoomJoined>(lobby, alice, RoomJoin{});

  ASSERT_TRUE(joined.has_value());
  EXPECT_NE(joined->roomId, first->roomId);
  EXPECT_EQ(lobby.partyOf(alice.endpoint()), lobby.partyOf(bob.endpoint()));
}

TEST(Lobby, AnswersARepeatedRoomLeave) {
  ServerNetwork network(0);
  Lobby lobby(network);
  TestClient client(network);
  ASSERT_TRUE(connect(lobby, client).has_value());

  // Its first ROOM_LEFT was lost: it is already in the lobby.
  EXPECT_TRUE(ask<RoomLeft>(lobby, client, RoomLeave{}).has_value());
}

TEST(Lobby, ForgetsAPlayerThatQuitsTheServer) {
  ServerNetwork network(0);
  Lobby lobby(network);
  TestClient client(network);
  ASSERT_TRUE(join(lobby, client).has_value());
  ASSERT_EQ(lobby.partyCount(), 1U);

  client.send(Disconnect{.reason = DisconnectReason::Quit});

  EXPECT_TRUE(stepUntil(lobby, [&lobby] { return lobby.partyCount() == 0; }));
  EXPECT_EQ(lobby.playerCount(), 0U);
  EXPECT_EQ(lobby.visitorCount(), 0U);
}

TEST(Lobby, TakesARestartedClientOutOfItsParty) {
  ServerNetwork network(0);
  Lobby lobby(network);
  TestClient client(network);
  ASSERT_TRUE(join(lobby, client).has_value());

  // Same address, but the client starts over: it wants to enter the lobby.
  client.send(ConnectRequest{.playerName = "Again"});
  const auto accept = stepUntil<ConnectAccept>(lobby, client, NO_ANSWER_WAIT);
  EXPECT_FALSE(accept.has_value());  // it was still in its party
  ASSERT_TRUE(connect(lobby, client, "Again").has_value());

  EXPECT_FALSE(lobby.partyOf(client.endpoint()).has_value());
  EXPECT_TRUE(stepUntil(lobby, [&lobby] { return lobby.partyCount() == 0; }));
}

TEST(Lobby, StopsEveryPartyWhenItShutsDown) {
  ServerNetwork network(0);
  Lobby lobby(network);
  const auto players = fillParty(lobby, network);
  TestClient late(network);
  ASSERT_TRUE(join(lobby, late).has_value());
  TestClient visitor(network);
  ASSERT_TRUE(connect(lobby, visitor).has_value());
  ASSERT_EQ(lobby.partyCount(), 2U);

  const std::atomic<bool> stopRequested{true};
  lobby.run(stopRequested);

  EXPECT_EQ(lobby.partyCount(), 0U);
  EXPECT_EQ(lobby.visitorCount(), 0U);
  for (TestClient* client : {players.front().get(), &late, &visitor}) {
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
