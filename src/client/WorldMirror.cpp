#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/ECSExceptions.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/physics/components/Velocity.hpp>
#include <ampersand/render/components/Sprite.hpp>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <rtype/client/WorldMirror.hpp>
#include <rtype/common/components/Kind.hpp>
#include <rtype/common/protocol/Packets.hpp>
#include <rtype/common/protocol/Protocol.hpp>

namespace rtype::client {

namespace core = ampersand::core;
namespace physics = ampersand::physics;
namespace render = ampersand::render;

WorldMirror::WorldMirror(core::ecs::World& world, const EntitySprites& sprites)
    : _world(world), _sprites(sprites) {}

void WorldMirror::spawn(const protocol::EntitySpawn& packet) {
  if (this->_entities.contains(packet.entityId) ||
      this->_destroyed.contains(packet.entityId)) {
    return;
  }
  core::ecs::Entity entity;
  try {
    entity = this->_world.create();
  } catch (const core::ecs::EntityLimitReached&) {
    return;
  }
  const float scale = this->_sprites.scale;
  this->_world.add(entity, core::Transform{
                               .position = {packet.x, packet.y, 0.0F},
                               .scale = {scale, scale, 1.0F},
                           });
  this->_world.add(entity, physics::Velocity{
                               .value = {packet.velocityX, packet.velocityY},
                           });
  this->_world.add(entity, this->spriteFor(packet));
  this->_entities.emplace(packet.entityId, entity);
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
    auto& position = this->_world.get<core::Transform>(found->second).position;
    position.x = state.x;
    position.y = state.y;
    this->_world.get<physics::Velocity>(found->second).value = {
        state.velocityX, state.velocityY};
  }
}

void WorldMirror::destroy(const protocol::EntityDestroy& packet) {
  this->_destroyed.insert(packet.entityId);
  const auto found = this->_entities.find(packet.entityId);
  if (found == this->_entities.end()) {
    return;
  }
  this->_world.destroy(found->second);
  this->_entities.erase(found);
}

std::optional<core::ecs::Entity> WorldMirror::find(
    std::uint32_t entityId) const {
  const auto found = this->_entities.find(entityId);
  if (found == this->_entities.end()) {
    return std::nullopt;
  }
  return found->second;
}

std::size_t WorldMirror::size() const noexcept {
  return this->_entities.size();
}

const render::Sprite& WorldMirror::spriteFor(
    const protocol::EntitySpawn& packet) const {
  switch (packet.entityType) {
    case common::EntityKind::Enemy:
      return this->_sprites.enemy;
    case common::EntityKind::PlayerMissile:
      return this->_sprites.playerMissile;
    case common::EntityKind::EnemyMissile:
      return this->_sprites.enemyMissile;
    case common::EntityKind::PowerUp:
      return this->_sprites.powerUp;
    case common::EntityKind::Obstacle:
      return this->_sprites.obstacle;
    case common::EntityKind::Ship:
      break;
  }
  return this->_sprites.ships[packet.ownerId % protocol::MAX_PLAYERS];
}

}  // namespace rtype::client
