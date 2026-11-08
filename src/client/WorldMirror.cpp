#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/physics/components/BoxCollider.hpp>
#include <ampersand/physics/components/Velocity.hpp>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <rtype/client/IEntityFactory.hpp>
#include <rtype/client/WorldMirror.hpp>
#include <rtype/client/components/Correction.hpp>
#include <rtype/client/components/Predicted.hpp>
#include <rtype/common/components/Kind.hpp>
#include <rtype/common/protocol/Packets.hpp>
#include <rtype/common/resources/Deaths.hpp>

namespace rtype::client {

namespace core = ampersand::core;
namespace physics = ampersand::physics;

WorldMirror::WorldMirror(core::ecs::World& world, IEntityFactory& factory)
    : _world(world), _factory(factory) {}

void WorldMirror::setLocalPlayer(std::optional<std::uint8_t> playerId) {
  this->_localPlayer = playerId;
}

void WorldMirror::spawn(const protocol::EntitySpawn& packet) {
  if (this->_entities.contains(packet.entityId) ||
      this->_destroyed.contains(packet.entityId)) {
    return;
  }
  const std::optional<core::ecs::Entity> created =
      this->_factory.create(packet.entityType, packet.variant, packet.ownerId);
  if (!created) {
    return;
  }
  const core::ecs::Entity entity = *created;
  auto& position = this->_world.get<core::Transform>(entity).position;
  position.x = packet.x;
  position.y = packet.y;
  this->_world.add(entity, Correction{});
  if (this->_world.has<physics::Velocity>(entity)) {
    this->_world.remove<physics::Velocity>(entity);
  }
  if (this->isLocalShip(packet)) {
    this->_world.add(entity,
                     Predicted{
                         .serverPosition = {packet.x, packet.y},
                         .serverVelocity = {packet.velocityX, packet.velocityY},
                     });
  } else {
    this->_world.add(entity, physics::Velocity{
                                 .value = {packet.velocityX, packet.velocityY},
                             });
  }
  this->_entities.emplace(packet.entityId, Mirrored{
                                               .entity = entity,
                                               .kind = packet.entityType,
                                               .variant = packet.variant,
                                               .ownerId = packet.ownerId,
                                           });
}

void WorldMirror::applySnapshot(const protocol::WorldSnapshot& packet) {
  if (packet.serverTick < this->_lastSnapshotTick) {
    return;
  }
  this->_lastSnapshotTick = packet.serverTick;
  for (const protocol::EntityState& state : packet.entities) {
    const auto found = this->_entities.find(state.entityId);
    if (found == this->_entities.end()) {
      continue;
    }
    const core::ecs::Entity entity = found->second.entity;
    if (this->_world.has<Predicted>(entity)) {
      this->_world.get<Predicted>(entity) = Predicted{
          .serverPosition = {state.x, state.y},
          .serverVelocity = {state.velocityX, state.velocityY},
      };
      continue;
    }
    this->correct(entity, {state.x, state.y});
    this->_world.get<physics::Velocity>(entity).value = {state.velocityX,
                                                         state.velocityY};
  }
}

void WorldMirror::destroy(const protocol::EntityDestroy& packet) {
  this->_destroyed.insert(packet.entityId);
  const auto found = this->_entities.find(packet.entityId);
  if (found == this->_entities.end()) {
    return;
  }
  if (packet.reason == common::DeathReason::Killed) {
    this->die(found->second);
  }
  this->_world.destroy(found->second.entity);
  this->_entities.erase(found);
}

void WorldMirror::clear() {
  for (const auto& [entityId, mirrored] : this->_entities) {
    this->_world.destroy(mirrored.entity);
  }
  this->_entities.clear();
  this->_destroyed.clear();
  this->_lastSnapshotTick = 0;
}

std::optional<core::ecs::Entity> WorldMirror::find(
    std::uint32_t entityId) const {
  const auto found = this->_entities.find(entityId);
  if (found == this->_entities.end()) {
    return std::nullopt;
  }
  return found->second.entity;
}

std::size_t WorldMirror::size() const noexcept {
  return this->_entities.size();
}

bool WorldMirror::isLocalShip(const protocol::EntitySpawn& packet) const {
  return packet.entityType == common::EntityKind::Ship &&
         this->_localPlayer == packet.ownerId;
}

// Small errors are smoothed by the CorrectionSystem, big ones are jumped.
void WorldMirror::correct(core::ecs::Entity entity,
                          core::math::Vec2f serverPosition) {
  auto& position = this->_world.get<core::Transform>(entity).position;
  auto& offset = this->_world.get<Correction>(entity).offset;
  offset = {serverPosition.x - position.x, serverPosition.y - position.y};
  if (std::hypot(offset.x, offset.y) > SNAP_DISTANCE) {
    position.x = serverPosition.x;
    position.y = serverPosition.y;
    offset = {};
  }
}

void WorldMirror::die(const Mirrored& mirrored) {
  const auto& dead =
      this->_world.get<core::Transform>(mirrored.entity).position;
  const auto& bounds =
      this->_world.get<physics::BoxCollider>(mirrored.entity).bounds;
  const core::math::Vec2f center{
      dead.x + bounds.x + (bounds.width / 2.0F),
      dead.y + bounds.y + (bounds.height / 2.0F),
  };
  const std::optional<core::ecs::Entity> effect = this->_factory.createDeath(
      mirrored.kind, mirrored.variant, mirrored.ownerId);
  if (!effect) {
    return;
  }
  auto& position = this->_world.get<core::Transform>(*effect).position;
  position.x = center.x;
  position.y = center.y;
}

}  // namespace rtype::client
