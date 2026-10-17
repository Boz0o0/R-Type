#include <gtest/gtest.h>

#include <ampersand/core/ecs/World.hpp>
#include <rtype/client/Hud.hpp>
#include <rtype/client/ServerConnection.hpp>
#include <rtype/common/protocol/Packets.hpp>
#include <rtype/common/protocol/Protocol.hpp>

using ampersand::core::ecs::World;
using rtype::client::Hud;
using State = rtype::client::ServerConnection::State;
using rtype::protocol::DisconnectReason;
using rtype::protocol::GameOver;
using rtype::protocol::GameResult;
using rtype::protocol::PlayerJoined;
using rtype::protocol::PlayerLeft;
using rtype::protocol::PlayerState;
using rtype::protocol::RejectReason;

TEST(Hud, ShowsConnectingThenOurPlayer) {
  World world;
  Hud hud(world, 0);

  hud.showConnection(State::Connecting, std::nullopt, std::nullopt);
  EXPECT_EQ(hud.status(), "Connecting...");

  hud.showConnection(State::Connected, 2, std::nullopt);
  EXPECT_EQ(hud.status(), "You: P3");
  EXPECT_TRUE(hud.banner().empty());
}

TEST(Hud, ShowsWhyTheServerRefusedUs) {
  World world;
  Hud hud(world, 0);

  hud.showConnection(State::Rejected, std::nullopt, RejectReason::ServerFull);

  EXPECT_EQ(hud.banner(), "Server full");
}

TEST(Hud, ShowsOnlyOurOwnStats) {
  World world;
  Hud hud(world, 0);
  hud.showConnection(State::Connected, 0, std::nullopt);

  hud.apply(PlayerState{.playerId = 1, .health = 1, .lives = 1, .score = 5});
  EXPECT_TRUE(hud.stats().empty());

  hud.apply(PlayerState{.playerId = 0, .health = 3, .lives = 2, .score = 150});
  EXPECT_EQ(hud.stats(), "HP 3   Lives 2   Score 150");
}

TEST(Hud, ShowsWhoJoinedAndWhoLeft) {
  World world;
  Hud hud(world, 0);

  hud.apply(PlayerJoined{.playerId = 1, .playerName = "Bob"});
  EXPECT_EQ(hud.event(), "P2 Bob joined");

  hud.apply(PlayerLeft{.playerId = 1, .reason = DisconnectReason::Timeout});
  EXPECT_EQ(hud.event(), "P2 lost connection");

  hud.apply(PlayerLeft{.playerId = 3, .reason = DisconnectReason::Quit});
  EXPECT_EQ(hud.event(), "P4 left");
}

// The server disconnects everyone after the game: keep the result on screen.
TEST(Hud, GameOverStaysWhenTheConnectionCloses) {
  World world;
  Hud hud(world, 0);

  hud.apply(GameOver{.result = GameResult::Victory, .finalScore = 900});
  hud.showConnection(State::Disconnected, std::nullopt, std::nullopt);

  EXPECT_EQ(hud.banner(), "VICTORY   Score 900");
}
