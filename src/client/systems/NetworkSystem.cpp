#include <ampersand/core/ecs/World.hpp>
#include <ampersand/input/resources/InputManager.hpp>
#include <rtype/client/Controls.hpp>
#include <rtype/client/Hud.hpp>
#include <rtype/client/ServerConnection.hpp>
#include <rtype/client/WorldMirror.hpp>
#include <rtype/client/systems/NetworkSystem.hpp>
#include <rtype/common/protocol/PacketCodec.hpp>
#include <rtype/common/protocol/Packets.hpp>
#include <variant>

namespace rtype::client {

NetworkSystem::NetworkSystem(ServerConnection& connection,
                             WorldMirror& worldMirror, Hud& hud)
    : _connection(connection), _worldMirror(worldMirror), _hud(hud) {}

void NetworkSystem::update(ampersand::core::ecs::World& world) {
  this->_connection.update(ServerConnection::Clock::now());
  this->_hud.showConnection(this->_connection.state(),
                            this->_connection.playerId(),
                            this->_connection.rejectReason());
  for (const protocol::Payload& packet : this->_connection.takePackets()) {
    applyPacket(this->_worldMirror, this->_hud, packet);
  }
  this->_connection.sendInput(
      Controls::inputMask(world.getResource<ampersand::input::InputManager>()));
}

void NetworkSystem::applyPacket(WorldMirror& worldMirror, Hud& hud,
                                const protocol::Payload& packet) {
  hud.apply(packet);
  if (const auto* spawn = std::get_if<protocol::EntitySpawn>(&packet)) {
    worldMirror.spawn(*spawn);
  } else if (const auto* snapshot =
                 std::get_if<protocol::WorldSnapshot>(&packet)) {
    worldMirror.applySnapshot(*snapshot);
  } else if (const auto* destroyed =
                 std::get_if<protocol::EntityDestroy>(&packet)) {
    worldMirror.destroy(*destroyed);
  }
}

}  // namespace rtype::client
