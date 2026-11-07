#pragma once

#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/scene/resources/SceneManager.hpp>
#include <array>
#include <cstdint>
#include <optional>
#include <rtype/client/IEntityFactory.hpp>
#include <rtype/common/components/Kind.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <string>
#include <string_view>

namespace rtype::client {

class PrefabFactory final : public IEntityFactory {
 public:
  PrefabFactory(ampersand::core::ecs::World& world,
                ampersand::scene::SceneManager& scenes);

  [[nodiscard]] std::optional<ampersand::core::ecs::Entity> create(
      common::EntityKind kind, std::uint8_t variant,
      std::uint8_t ownerId) override;
  [[nodiscard]] std::optional<ampersand::core::ecs::Entity> createExplosion()
      override;

 private:
  static constexpr std::array<std::string_view, protocol::MAX_PLAYERS> SHIPS{
      "world/prefabs/ship-1.json",
      "world/prefabs/ship-2.json",
      "world/prefabs/ship-3.json",
      "world/prefabs/ship-4.json",
  };
  static constexpr std::array<std::string_view, 2> ENEMIES{
      "world/prefabs/enemy.json",
      "world/prefabs/enemy-sine.json",
  };
  static constexpr std::string_view PLAYER_MISSILE =
      "world/prefabs/player-missile.json";
  static constexpr std::string_view ENEMY_MISSILE =
      "world/prefabs/enemy-missile.json";
  static constexpr std::string_view EXPLOSION = "world/prefabs/explosion.json";

  // Power-ups and obstacles have no look yet.
  [[nodiscard]] static std::optional<std::string_view> prefabOf(
      common::EntityKind kind, std::uint8_t variant, std::uint8_t ownerId);
  [[nodiscard]] std::optional<ampersand::core::ecs::Entity> instantiate(
      std::string_view path);
  [[nodiscard]] std::optional<ampersand::core::ecs::Entity> createPlain();

  ampersand::core::ecs::World& _world;
  ampersand::scene::SceneManager& _scenes;
};

}  // namespace rtype::client
