#include <gtest/gtest.h>

#include <ampersand/core/ecs/World.hpp>
#include <ampersand/input/Event.hpp>
#include <ampersand/input/resources/InputManager.hpp>
#include <rtype/client/Controls.hpp>
#include <rtype/client/LobbyMenu.hpp>
#include <rtype/client/ServerConnection.hpp>
#include <rtype/client/systems/LobbySystem.hpp>
#include <rtype/common/protocol/Packets.hpp>
#include <string>

#include "FakeServer.hpp"

using ampersand::core::ecs::World;
using ampersand::input::InputManager;
using ampersand::input::KeyCode;
using ampersand::input::KeyPressed;
using rtype::client::Controls;
using rtype::client::LobbyMenu;
using rtype::client::LobbySystem;
using rtype::client::ServerConnection;
using rtype::client::test::connectToLobby;
using rtype::client::test::FakeServer;
using rtype::client::test::START;
using State = ServerConnection::State;
namespace protocol = rtype::protocol;

// Our client in the lobby, with no key pressed yet.
class LobbySystemTest : public testing::Test {
 protected:
  void SetUp() override {
    connectToLobby(this->server, this->connection);
    Controls::bind(this->world.addResource<InputManager>());
    this->input().beginFrame();
  }

  InputManager& input() { return this->world.getResource<InputManager>(); }

  FakeServer server;
  ServerConnection connection{"127.0.0.1", this->server.socket.localPort()};
  World world;
  LobbyMenu menu{this->world, 0};
};

TEST_F(LobbySystemTest, JoinsThePartyCodeOfTheCommandLineOnce) {
  LobbySystem system(this->connection, this->menu, "1234");

  system.update(this->world);
  this->connection.update(START);

  EXPECT_EQ(this->connection.state(), State::Joining);
  EXPECT_EQ(this->server.receive<protocol::RoomJoin>().code, "1234");
}

TEST_F(LobbySystemTest, SendsTheRoomChosenInTheMenu) {
  LobbySystem system(this->connection, this->menu, {});
  this->input().apply(KeyPressed{.key = KeyCode::Return});

  system.update(this->world);
  this->connection.update(START);

  const auto request = this->server.receive<protocol::RoomJoin>();
  EXPECT_EQ(request.roomId, 0U);
  EXPECT_TRUE(request.code.empty());
}

TEST_F(LobbySystemTest, ShowsTheMenuInTheLobby) {
  LobbySystem system(this->connection, this->menu, {});

  system.update(this->world);

  EXPECT_EQ(this->menu.lines().front(), "LOBBY");
}
