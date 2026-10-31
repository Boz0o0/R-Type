#include <ampersand/audio/SoundId.hpp>
#include <ampersand/audio/components/AudioSource.hpp>
#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/ECSExceptions.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/physics/components/Velocity.hpp>
#include <ampersand/render/components/Animation.hpp>
#include <ampersand/render/components/Sprite.hpp>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <rtype/client/EntityLooks.hpp>
#include <rtype/client/WorldMirror.hpp>
#include <rtype/client/components/Explosion.hpp>
#include <rtype/common/components/Kind.hpp>
#include <rtype/common/protocol/Packets.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <rtype/common/resources/Deaths.hpp>
#include <utility>

namespace rtype::client {

namespace audio = ampersand::audio;
namespace core = ampersand::core;
namespace physics = ampersand::physics;
namespace render = ampersand::render;

WorldMirror::WorldMirror(core::ecs::World& world, EntityLooks looks)
    : _world(world), _looks(std::move(looks)) {}

void WorldMirror::spawn(const protocol::EntitySpawn& packet) {
  if (this->_entities.contains(packet.entityId) ||
      this->_destroyed.contains(packet.entityId)) {
    return;
  }
  const std::optional<core::ecs::Entity> created =
      this->create(this->lookFor(packet), {packet.x, packet.y});
  if (!created) {
    return;
  }
  const core::ecs::Entity entity = *created;
  this->_world.add(entity, physics::Velocity{
                               .value = {packet.velocityX, packet.velocityY},
                           });
  if (packet.entityType == common::EntityKind::PlayerMissile) {
    this->play(entity, this->_looks.shootSound, SHOOT_VOLUME);
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
    auto& position = this->_world.get<core::Transform>(entity).position;
    position.x = state.x;
    position.y = state.y;
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

const Look& WorldMirror::lookFor(const protocol::EntitySpawn& packet) const {
  switch (packet.entityType) {
    case common::EntityKind::Enemy:
      return this->_looks.enemy;
    case common::EntityKind::PlayerMissile:
      return this->_looks.playerMissile;
    case common::EntityKind::EnemyMissile:
      return this->_looks.enemyMissile;
    case common::EntityKind::PowerUp:
      return this->_looks.powerUp;
    case common::EntityKind::Obstacle:
      return this->_looks.obstacle;
    case common::EntityKind::Ship:
      break;
  }
  return this->_looks.ships[packet.ownerId % protocol::MAX_PLAYERS];
}

std::optional<core::ecs::Entity> WorldMirror::create(
    const Look& look, core::math::Vec2f position) {
  core::ecs::Entity entity;
  try {
    entity = this->_world.create();
  } catch (const core::ecs::EntityLimitReached&) {
    return std::nullopt;
  }
  this->_world.add(entity, core::Transform{
                               .position = {position.x, position.y, 0.0F},
                               .scale = {look.scale, look.scale, 1.0F},
                           });
  render::Sprite sprite = look.sprite;
  if (!look.animation.frames.empty()) {
    sprite.source = look.animation.frames.front();
    this->_world.add(entity, look.animation);
  }
  this->_world.add(entity, sprite);
  return entity;
}

// Centers the explosion on the sprite of the entity that died.
void WorldMirror::explode(const core::Transform& transform,
                          const render::Sprite& sprite) {
  const Look& look = this->_looks.explosion;
  const auto& frame = look.animation.frames.empty()
                          ? look.sprite.source
                          : look.animation.frames.front();
  const float centerX =
      transform.position.x +
      (static_cast<float>(sprite.source.width) * transform.scale.x / 2.0F);
  const float centerY =
      transform.position.y +
      (static_cast<float>(sprite.source.height) * transform.scale.y / 2.0F);
  const std::optional<core::ecs::Entity> created = this->create(
      look, {centerX - (static_cast<float>(frame.width) * look.scale / 2.0F),
             centerY - (static_cast<float>(frame.height) * look.scale / 2.0F)});
  if (!created) {
    return;
  }
  const core::ecs::Entity explosion = *created;
  this->_world.get<core::Transform>(explosion).position.z = EXPLOSION_LAYER;
  this->_world.add(explosion, Explosion{});
  this->play(explosion, this->_looks.explosionSound, EXPLOSION_VOLUME);
}

void WorldMirror::play(core::ecs::Entity entity, audio::SoundId sound,
                       float volume) {
  if (sound == 0) {
    return;
  }
  this->_world.add(entity, audio::AudioSource{
                               .source = audio::SoundRef{.id = sound},
                               .volume = volume,
                               .playing = true,
                           });
}

}  // namespace rtype::client
