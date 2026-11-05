#include <ampersand/core/ecs/EventQueue.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/input/resources/InputManager.hpp>
#include <rtype/client/Controls.hpp>
#include <rtype/client/ServerConnection.hpp>
#include <rtype/client/WorldMirror.hpp>
#include <rtype/client/systems/NetworkSystem.hpp>
#include <rtype/common/protocol/PacketCodec.hpp>
#include <rtype/common/protocol/Packets.hpp>
#include <utility>
#include <variant>

namespace rtype::client {

NetworkSystem::NetworkSystem(ServerConnection& connection,
                             WorldMirror& worldMirror)
    : _connection(connection),
      _worldMirror(worldMirror),
      _mirroredRoom(connection.roomGeneration()) {}

void NetworkSystem::update(ampersand::core::ecs::World& world) {
  this->_connection.update(ServerConnection::Clock::now());
  if (this->_connection.roomGeneration() != this->_mirroredRoom) {
    this->_worldMirror.clear();
    this->_mirroredRoom = this->_connection.roomGeneration();
  }
  this->_worldMirror.setLocalPlayer(this->_connection.playerId());
  auto& packets =
      world.getResource<ampersand::core::ecs::EventQueue<protocol::Payload>>();
  for (protocol::Payload& packet : this->_connection.takePackets()) {
    applyPacket(this->_worldMirror, packet);
    packets.send(std::move(packet));
  }
  this->_connection.sendInput(
      Controls::inputMask(world.getResource<ampersand::input::InputManager>()));
}

void NetworkSystem::applyPacket(WorldMirror& worldMirror,
                                const protocol::Payload& packet) {
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
