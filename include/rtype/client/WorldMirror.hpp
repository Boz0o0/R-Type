#pragma once

#include <ampersand/audio/SoundId.hpp>
#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/render/components/Sprite.hpp>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <rtype/client/EntityLooks.hpp>
#include <rtype/common/components/Kind.hpp>
#include <rtype/common/protocol/Packets.hpp>
#include <unordered_map>
#include <unordered_set>

namespace rtype::client {

// Keeps one local entity for each entity the server told us about.
class WorldMirror {
 public:
  WorldMirror(ampersand::core::ecs::World& world, EntityLooks looks);

  void spawn(const protocol::EntitySpawn& packet);
  void applySnapshot(const protocol::WorldSnapshot& packet);
  void destroy(const protocol::EntityDestroy& packet);

  [[nodiscard]] std::optional<ampersand::core::ecs::Entity> find(
      std::uint32_t entityId) const;
  [[nodiscard]] std::size_t size() const noexcept;

 private:
  static constexpr float SHOOT_VOLUME = 40.0F;
  static constexpr float EXPLOSION_VOLUME = 70.0F;
  // Drawn over the ships and the missiles.
  static constexpr float EXPLOSION_LAYER = 1.0F;

  struct Mirrored {
    ampersand::core::ecs::Entity entity;
    common::EntityKind kind;
  };

  [[nodiscard]] const Look& lookFor(const protocol::EntitySpawn& packet) const;
  ampersand::core::ecs::Entity create(const Look& look,
                                      ampersand::core::math::Vec2f position);
  void explode(const ampersand::core::Transform& transform,
               const ampersand::render::Sprite& sprite);
  void play(ampersand::core::ecs::Entity entity,
            ampersand::audio::SoundId sound, float volume);

  ampersand::core::ecs::World& _world;
  EntityLooks _looks;
  std::unordered_map<std::uint32_t, Mirrored> _entities;
  // Ids are never reused, so a spawn arriving after its destroy is dropped.
  std::unordered_set<std::uint32_t> _destroyed;
  std::uint32_t _lastSnapshotTick{0};
};

}  // namespace rtype::client
