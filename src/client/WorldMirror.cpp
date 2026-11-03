#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/physics/components/Velocity.hpp>
#include <ampersand/render/components/Sprite.hpp>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <rtype/client/IEntityFactory.hpp>
#include <rtype/client/WorldMirror.hpp>
#include <rtype/client/components/Correction.hpp>
#include <rtype/client/components/Explosion.hpp>
#include <rtype/client/components/Predicted.hpp>
#include <rtype/common/components/Kind.hpp>
#include <rtype/common/protocol/Packets.hpp>
#include <rtype/common/resources/Deaths.hpp>

namespace rtype::client {

namespace core = ampersand::core;
namespace physics = ampersand::physics;
namespace render = ampersand::render;

WorldMirror::WorldMirror(core::ecs::World& world, IEntityFactory& factory)
    : _world(world), _factory(factory) {}

void WorldMirror::setLocalPlayer(std::uint8_t playerId) {
  this->_localPlayer = playerId;
}

void WorldMirror::spawn(const protocol::EntitySpawn& packet) {
  if (this->_entities.contains(packet.entityId) ||
      this->_destroyed.contains(packet.entityId)) {
    return;
  }
  const std::optional<core::ecs::Entity> created =
      this->_factory.create(packet.entityType, packet.ownerId);
  if (!created) {
    return;
  }
  const core::ecs::Entity entity = *created;
  auto& position = this->_world.get<core::Transform>(entity).position;
  position.x = packet.x;
  position.y = packet.y;
  this->_world.add(entity, Correction{});
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
  this->_entities.emplace(
      packet.entityId, Mirrored{.entity = entity, .kind = packet.entityType});
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
  const auto [entity, kind] = found->second;
  const bool isShipOrEnemy =
      kind == common::EntityKind::Ship || kind == common::EntityKind::Enemy;
  if (packet.reason == common::DeathReason::Killed && isShipOrEnemy) {
    // Copies: creating the explosion can move the component storage.
    const core::Transform transform = this->_world.get<core::Transform>(entity);
    const render::Sprite sprite = this->_world.get<render::Sprite>(entity);
    this->explode(transform, sprite);
  }
  this->_world.destroy(entity);
  this->_entities.erase(found);
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

// Centers the explosion on the sprite of the entity that died.
void WorldMirror::explode(const core::Transform& transform,
                          const render::Sprite& sprite) {
  const std::optional<core::ecs::Entity> created =
      this->_factory.createExplosion();
  if (!created) {
    return;
  }
  const core::ecs::Entity explosion = *created;
  this->_world.add(explosion, Explosion{});
  const auto& frame = this->_world.get<render::Sprite>(explosion).source;
  auto& placed = this->_world.get<core::Transform>(explosion);
  const float centerX =
      transform.position.x +
      (static_cast<float>(sprite.source.width) * transform.scale.x / 2.0F);
  const float centerY =
      transform.position.y +
      (static_cast<float>(sprite.source.height) * transform.scale.y / 2.0F);
  placed.position.x =
      centerX - (static_cast<float>(frame.width) * placed.scale.x / 2.0F);
  placed.position.y =
      centerY - (static_cast<float>(frame.height) * placed.scale.y / 2.0F);
}

}  // namespace rtype::client
