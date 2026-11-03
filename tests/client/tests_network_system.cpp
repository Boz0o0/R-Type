#include <gtest/gtest.h>

#include <ampersand/core/ecs/World.hpp>
#include <ampersand/input/Event.hpp>
#include <ampersand/input/resources/InputManager.hpp>
#include <chrono>
#include <rtype/client/Hud.hpp>
#include <rtype/client/ServerConnection.hpp>
#include <rtype/client/WorldMirror.hpp>
#include <rtype/client/systems/NetworkSystem.hpp>
#include <rtype/common/components/Kind.hpp>
#include <rtype/common/protocol/Packets.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <thread>
#include <variant>

#include "FakeEntityFactory.hpp"
#include "FakeServer.hpp"
#include "GameFiles.hpp"

using ampersand::core::ecs::World;
using ampersand::input::InputManager;
using ampersand::input::KeyCode;
using ampersand::input::KeyPressed;
using rtype::client::Hud;
using rtype::client::NetworkSystem;
using rtype::client::ServerConnection;
using rtype::client::WorldMirror;
using rtype::client::test::connectToServer;
using rtype::client::test::FakeEntityFactory;
using rtype::client::test::FakeServer;
using rtype::client::test::gameInput;
using rtype::common::EntityKind;
namespace protocol = rtype::protocol;

TEST(NetworkSystem, MirrorsTheServerAndSendsTheInput) {
  FakeServer server;
  ServerConnection connection("127.0.0.1", server.socket.localPort(), "Bob");
  connectToServer(server, connection);
  World world;
  auto& manager = world.addResource<InputManager>(gameInput());
  manager.apply(KeyPressed{.key = KeyCode::Space});
  FakeEntityFactory factory(world);
  WorldMirror worldMirror(world, factory);
  Hud hud(world, 0);
  NetworkSystem system(connection, worldMirror, hud);

  server.send(
      protocol::EntitySpawn{.entityId = 7, .entityType = EntityKind::Enemy}, 5);
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(1);
  while (worldMirror.size() == 0 &&
         std::chrono::steady_clock::now() < deadline) {
    system.update(world);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  EXPECT_TRUE(worldMirror.find(7).has_value());
  bool sentShoot = false;
  for (int packets = 0; packets < 10 && !sentShoot; ++packets) {
    const protocol::Packet packet = server.receive();
    if (const auto* input =
            std::get_if<protocol::PlayerInput>(&packet.payload)) {
      sentShoot = input->inputMask == protocol::InputFlag::SHOOT;
    }
  }
  EXPECT_TRUE(sentShoot);
}
