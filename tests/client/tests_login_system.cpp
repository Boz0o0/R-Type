#include <gtest/gtest.h>

#include <ampersand/core/ecs/World.hpp>
#include <ampersand/input/Event.hpp>
#include <ampersand/input/resources/InputManager.hpp>
#include <rtype/client/LoginMenu.hpp>
#include <rtype/client/ServerConnection.hpp>
#include <rtype/client/systems/LoginSystem.hpp>
#include <rtype/common/protocol/Packets.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <string>

#include "FakeServer.hpp"
#include "GameFiles.hpp"

using ampersand::core::ecs::World;
using ampersand::input::InputManager;
using ampersand::input::KeyCode;
using ampersand::input::KeyPressed;
using rtype::client::LoginMenu;
using rtype::client::LoginSystem;
using rtype::client::ServerConnection;
using rtype::client::test::FakeServer;
using rtype::client::test::gameInput;
using rtype::client::test::START;
using rtype::client::test::updateUntilStateChanges;
using State = ServerConnection::State;
namespace protocol = rtype::protocol;

// Our client logged out, "Alice" typed in already.
class LoginSystemTest : public testing::Test {
 protected:
  void SetUp() override {
    this->world.addResource<InputManager>(gameInput());
    this->input().beginFrame();
  }

  InputManager& input() { return this->world.getResource<InputManager>(); }

  /** @brief Presses the keys in one frame each, the system running. */
  void press(std::initializer_list<KeyCode> keys) {
    for (const KeyCode key : keys) {
      this->input().beginFrame();
      this->input().apply(KeyPressed{.key = key});
      this->system.update(this->world);
    }
  }

  FakeServer server;
  ServerConnection connection{"127.0.0.1", this->server.socket.localPort()};
  World world;
  LoginMenu menu{this->world, 0, "Alice"};
  LoginSystem system{this->connection, this->menu};
};

TEST_F(LoginSystemTest, ShowsTheMenuWhileLoggedOut) {
  this->system.update(this->world);

  EXPECT_EQ(this->menu.lines().front(), "R-TYPE");
}

TEST_F(LoginSystemTest, LogsInWithWhatWasTyped) {
  this->press({KeyCode::P, KeyCode::W, KeyCode::Return});
  this->connection.update(START);

  EXPECT_EQ(this->connection.state(), State::Connecting);
  const auto request = this->server.receive<protocol::Login>();
  EXPECT_EQ(request.username, "Alice");
  EXPECT_EQ(request.password, "pw");
  EXPECT_EQ(this->menu.lines().at(5), "Connecting...");
}

TEST_F(LoginSystemTest, ShowsWhyTheAccountWasRefused) {
  this->press({KeyCode::P, KeyCode::W, KeyCode::Return});
  this->connection.update(START);
  this->server.receive<protocol::Login>();

  this->server.send(
      protocol::ConnectReject{.reason = protocol::RejectReason::AccountExists});
  updateUntilStateChanges(this->connection, START);
  this->input().beginFrame();  // a new frame, without key
  this->system.update(this->world);

  EXPECT_EQ(this->connection.state(), State::LoggedOut);
  EXPECT_EQ(this->menu.lines().at(5), "Username already taken");
}

TEST_F(LoginSystemTest, HidesTheMenuInTheLobby) {
  this->press({KeyCode::P, KeyCode::W, KeyCode::Return});
  this->connection.update(START);
  this->server.receive<protocol::Login>();

  this->server.send(protocol::ConnectAccept{});
  updateUntilStateChanges(this->connection, START);
  this->input().beginFrame();
  this->system.update(this->world);

  EXPECT_EQ(this->connection.state(), State::InLobby);
  EXPECT_TRUE(this->menu.lines().front().empty());
}
