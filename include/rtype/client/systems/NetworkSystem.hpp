#pragma once

#include <ampersand/core/ecs/ISystem.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <rtype/client/Hud.hpp>
#include <rtype/client/ServerConnection.hpp>
#include <rtype/client/WorldMirror.hpp>
#include <rtype/common/protocol/PacketCodec.hpp>

namespace rtype::client {

class NetworkSystem final : public ampersand::core::ecs::ISystem {
 public:
  NetworkSystem(ServerConnection& connection, WorldMirror& worldMirror,
                Hud& hud);
  void update(ampersand::core::ecs::World& world) override;

 private:
  static void applyPacket(WorldMirror& worldMirror, Hud& hud,
                          const protocol::Payload& packet);

  ServerConnection& _connection;
  WorldMirror& _worldMirror;
  Hud& _hud;
};

}  // namespace rtype::client
