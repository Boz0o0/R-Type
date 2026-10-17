#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/physics/components/Velocity.hpp>
#include <ampersand/render/components/Sprite.hpp>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <rtype/client/WorldMirror.hpp>
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
  const float scale = this->_sprites.scale;
  const core::ecs::Entity entity = this->_world.create();
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
    case protocol::EntityType::Bydos:
      return this->_sprites.bydos;
    case protocol::EntityType::PlayerMissile:
      return this->_sprites.playerMissile;
    case protocol::EntityType::EnemyMissile:
      return this->_sprites.enemyMissile;
    case protocol::EntityType::PowerUp:
      return this->_sprites.powerUp;
    case protocol::EntityType::Obstacle:
      return this->_sprites.obstacle;
    case protocol::EntityType::Player:
      break;
  }
  return this->_sprites.players[packet.ownerId % protocol::MAX_PLAYERS];
}

}  // namespace rtype::client
