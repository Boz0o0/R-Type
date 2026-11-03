#pragma once

#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/render/components/Sprite.hpp>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <rtype/client/IEntityFactory.hpp>
#include <rtype/common/components/Kind.hpp>
#include <rtype/common/protocol/Packets.hpp>
#include <unordered_map>
#include <unordered_set>

namespace rtype::client {

// Keeps one local entity for each entity the server told us about.
class WorldMirror {
 public:
  WorldMirror(ampersand::core::ecs::World& world, IEntityFactory& factory);

  // Our own ship is then predicted instead of following the snapshots.
  void setLocalPlayer(std::uint8_t playerId);
  void spawn(const protocol::EntitySpawn& packet);
  void applySnapshot(const protocol::WorldSnapshot& packet);
  void destroy(const protocol::EntityDestroy& packet);

  [[nodiscard]] std::optional<ampersand::core::ecs::Entity> find(
      std::uint32_t entityId) const;
  [[nodiscard]] std::size_t size() const noexcept;

 private:
  // A bigger error is a jump (several lost snapshots): smoothing it would
  // show the entity flying across the screen.
  static constexpr float SNAP_DISTANCE = 128.0F;

  struct Mirrored {
    ampersand::core::ecs::Entity entity;
    common::EntityKind kind;
  };

  [[nodiscard]] bool isLocalShip(const protocol::EntitySpawn& packet) const;
  void correct(ampersand::core::ecs::Entity entity,
               ampersand::core::math::Vec2f serverPosition);
  void explode(const ampersand::core::Transform& transform,
               const ampersand::render::Sprite& sprite);

  ampersand::core::ecs::World& _world;
  IEntityFactory& _factory;
  std::unordered_map<std::uint32_t, Mirrored> _entities;
  // Ids are never reused, so a spawn arriving after its destroy is dropped.
  std::unordered_set<std::uint32_t> _destroyed;
  std::uint32_t _lastSnapshotTick{0};
  std::optional<std::uint8_t> _localPlayer;
};

}  // namespace rtype::client
