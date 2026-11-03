#include <gtest/gtest.h>

#include <ampersand/core/ecs/World.hpp>
#include <ampersand/input/Event.hpp>
#include <ampersand/input/resources/InputManager.hpp>
#include <chrono>
#include <rtype/client/Controls.hpp>
#include <rtype/client/EntityLooks.hpp>
#include <rtype/client/Hud.hpp>
#include <rtype/client/ServerConnection.hpp>
#include <rtype/client/WorldMirror.hpp>
#include <rtype/client/systems/NetworkSystem.hpp>
#include <rtype/common/components/Kind.hpp>
#include <rtype/common/protocol/Packets.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <thread>

#include "FakeServer.hpp"

using ampersand::core::ecs::World;
using ampersand::input::InputManager;
using ampersand::input::KeyCode;
using ampersand::input::KeyPressed;
using rtype::client::Controls;
using rtype::client::EntityLooks;
using rtype::client::Hud;
using rtype::client::NetworkSystem;
using rtype::client::ServerConnection;
using rtype::client::WorldMirror;
using rtype::client::test::enterRoom;
using rtype::client::test::FakeServer;
using rtype::client::test::JOINED_SEQUENCE;
using rtype::common::EntityKind;
namespace protocol = rtype::protocol;

// Our client in room 3, holding Space, with an enemy sent by the room.
class NetworkSystemTest : public testing::Test {
 protected:
  void SetUp() override {
    enterRoom(this->server, this->connection);
    auto& manager = this->world.addResource<InputManager>();
    Controls::bind(manager);
    manager.beginFrame();
    manager.apply(KeyPressed{.key = KeyCode::Space});
    this->server.send(
        protocol::EntitySpawn{.entityId = 7, .entityType = EntityKind::Enemy},
        JOINED_SEQUENCE + 1);
  }

  void updateUntilMirrored() {
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while (this->worldMirror.size() == 0 &&
           std::chrono::steady_clock::now() < deadline) {
      this->system.update(this->world);
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  FakeServer server;
  ServerConnection connection{"127.0.0.1", this->server.socket.localPort(),
                              "Bob"};
  World world;
  WorldMirror worldMirror{this->world, EntityLooks{}};
  Hud hud{this->world, 0};
  NetworkSystem system{this->connection, this->worldMirror, this->hud};
};

TEST_F(NetworkSystemTest, MirrorsTheRoomAndSendsTheInput) {
  this->updateUntilMirrored();

  EXPECT_TRUE(this->worldMirror.find(7).has_value());
  EXPECT_EQ(this->server.receive<protocol::PlayerInput>().inputMask,
            protocol::InputFlag::SHOOT);
}

TEST_F(NetworkSystemTest, StartsANewMirrorWhenWeLeaveTheRoom) {
  this->updateUntilMirrored();
  ASSERT_EQ(this->worldMirror.size(), 1U);

  this->connection.leaveRoom();
  this->system.update(this->world);

  EXPECT_EQ(this->worldMirror.size(), 0U);
}
