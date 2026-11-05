#pragma once

#include <ampersand/core/ecs/ISystem.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <cstdint>
#include <rtype/client/ServerConnection.hpp>
#include <rtype/client/WorldMirror.hpp>
#include <rtype/common/protocol/PacketCodec.hpp>

namespace rtype::client {

class NetworkSystem final : public ampersand::core::ecs::ISystem {
 public:
  NetworkSystem(ServerConnection& connection, WorldMirror& worldMirror);
  void update(ampersand::core::ecs::World& world) override;

 private:
  static void applyPacket(WorldMirror& worldMirror,
                          const protocol::Payload& packet);

  ServerConnection& _connection;
  WorldMirror& _worldMirror;
  // Each room numbers its entities from 1: a new room starts a new mirror.
  std::uint32_t _mirroredRoom;
};

}  // namespace rtype::client
