#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/ECSExceptions.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/scene/resources/SceneManager.hpp>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <rtype/client/PrefabFactory.hpp>
#include <rtype/common/components/Kind.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <stdexcept>
#include <string>
#include <string_view>

namespace rtype::client {

namespace core = ampersand::core;

PrefabFactory::PrefabFactory(core::ecs::World& world,
                             ampersand::scene::SceneManager& scenes)
    : _world(world), _scenes(scenes) {}

std::optional<core::ecs::Entity> PrefabFactory::create(common::EntityKind kind,
                                                       std::uint8_t variant,
                                                       std::uint8_t ownerId) {
  if (const auto prefab = prefabOf(kind, variant, ownerId)) {
    return this->instantiate(*prefab);
  }
  return this->createPlain();
}

std::optional<core::ecs::Entity> PrefabFactory::createDeath(
    common::EntityKind kind, std::uint8_t variant, std::uint8_t ownerId) {
  if (const auto prefab = deathOf(kind, variant, ownerId)) {
    return this->instantiate(*prefab);
  }
  return std::nullopt;
}

std::optional<std::string_view> PrefabFactory::deathOf(
    common::EntityKind kind,
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    std::uint8_t variant, std::uint8_t ownerId) {
  switch (kind) {
    case common::EntityKind::Ship:
      return SHIP_DEATHS[ownerId % protocol::MAX_PLAYERS];
    case common::EntityKind::Enemy:
      return ENEMY_DEATHS[variant < ENEMY_DEATHS.size() ? variant : 0];
    case common::EntityKind::PlayerMissile:
      return PLAYER_MISSILE_HIT;
    case common::EntityKind::EnemyMissile:
      return ENEMY_MISSILE_HIT;
    case common::EntityKind::PowerUp:
    case common::EntityKind::Obstacle:
      break;
  }
  return std::nullopt;
}

std::optional<std::string_view> PrefabFactory::prefabOf(
    common::EntityKind kind,
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    std::uint8_t variant, std::uint8_t ownerId) {
  switch (kind) {
    case common::EntityKind::Ship:
      return SHIPS[ownerId % protocol::MAX_PLAYERS];
    case common::EntityKind::Enemy:
      return ENEMIES[variant < ENEMIES.size() ? variant : 0];
    case common::EntityKind::PlayerMissile:
      return PLAYER_MISSILES[ownerId % protocol::MAX_PLAYERS];
    case common::EntityKind::EnemyMissile:
      return ENEMY_MISSILE;
    case common::EntityKind::PowerUp:
    case common::EntityKind::Obstacle:
      break;
  }
  return std::nullopt;
}

// A full World or a broken prefab only loses this entity: the game goes on.
// The tests load every prefab, so a broken one is found before a game.
std::optional<core::ecs::Entity> PrefabFactory::instantiate(
    std::string_view path) {
  try {
    return this->_scenes.instantiate(this->_world, std::string(path));
  } catch (const std::runtime_error& error) {
    std::fprintf(stderr, "[client] %s\n", error.what());
    return std::nullopt;
  }
}

std::optional<core::ecs::Entity> PrefabFactory::createPlain() {
  try {
    const core::ecs::Entity entity = this->_world.create();
    this->_world.add(entity, core::Transform{});
    return entity;
  } catch (const core::ecs::EntityLimitReached&) {
    return std::nullopt;
  }
}

}  // namespace rtype::client
