#pragma once

#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/render/components/Sprite.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <rtype/common/protocol/Packets.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <unordered_map>
#include <unordered_set>

namespace rtype::client {

struct EntitySprites {
  std::array<ampersand::render::Sprite, protocol::MAX_PLAYERS> ships;
  ampersand::render::Sprite enemy;
  ampersand::render::Sprite playerMissile;
  ampersand::render::Sprite enemyMissile;
  ampersand::render::Sprite powerUp;
  ampersand::render::Sprite obstacle;
  float scale{1.0F};
};

// Keeps one local entity for each entity the server told us about.
class WorldMirror {
 public:
  WorldMirror(ampersand::core::ecs::World& world, const EntitySprites& sprites);

  void spawn(const protocol::EntitySpawn& packet);
  void applySnapshot(const protocol::WorldSnapshot& packet);
  void destroy(const protocol::EntityDestroy& packet);

  [[nodiscard]] std::optional<ampersand::core::ecs::Entity> find(
      std::uint32_t entityId) const;
  [[nodiscard]] std::size_t size() const noexcept;

 private:
  [[nodiscard]] const ampersand::render::Sprite& spriteFor(
      const protocol::EntitySpawn& packet) const;

  ampersand::core::ecs::World& _world;
  EntitySprites _sprites;
  std::unordered_map<std::uint32_t, ampersand::core::ecs::Entity> _entities;
  // Ids are never reused, so a spawn arriving after its destroy is dropped.
  std::unordered_set<std::uint32_t> _destroyed;
  std::uint32_t _lastSnapshotTick{0};
};

}  // namespace rtype::client
