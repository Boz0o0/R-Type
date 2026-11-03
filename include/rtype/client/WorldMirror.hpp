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

/**
 * @brief Keeps one local entity for each entity the server told us about.
 *
 * ENTITY_SPAWN creates it with the look of its kind, WORLD_SNAPSHOT moves
 * it, and ENTITY_DESTROY removes it (with an explosion for a ship or an
 * enemy that was killed).
 */
class WorldMirror {
 public:
  /**
   * @brief Creates an empty mirror.
   *
   * @param world The world to create the local entities in.
   * @param looks Sprites and sounds of each kind of entity.
   */
  WorldMirror(ampersand::core::ecs::World& world, EntityLooks looks);

  /**
   * @brief Creates the local entity of a server entity. Ignored if it
   *        already exists, was already destroyed, or the world is full.
   *
   * @param packet The ENTITY_SPAWN received.
   */
  void spawn(const protocol::EntitySpawn& packet);

  /**
   * @brief Moves the known entities to their position and velocity in the
   *        snapshot. Ignored if it is older than the last one applied.
   *
   * @param packet The WORLD_SNAPSHOT received.
   */
  void applySnapshot(const protocol::WorldSnapshot& packet);

  /**
   * @brief Removes the local entity of a server entity, and remembers its id
   *        so that a late ENTITY_SPAWN does not bring it back.
   *
   * @param packet The ENTITY_DESTROY received.
   */
  void destroy(const protocol::EntityDestroy& packet);

  /** @brief Removes every entity, to mirror another room: each room numbers
   *         its entities and its ticks from the start. */
  void clear();

  /**
   * @brief Finds the local entity of a server entity.
   *
   * @param entityId Id of the entity on the server.
   * @return The local entity, if it is mirrored.
   */
  [[nodiscard]] std::optional<ampersand::core::ecs::Entity> find(
      std::uint32_t entityId) const;

  /** @brief Number of entities mirrored. */
  [[nodiscard]] std::size_t size() const noexcept;

 private:
  /** @brief Volume of the sound of a player shot. */
  static constexpr float SHOOT_VOLUME = 40.0F;
  /** @brief Volume of the sound of an explosion. */
  static constexpr float EXPLOSION_VOLUME = 70.0F;
  /** @brief Layer of the explosions: drawn over the ships and the missiles.
   */
  static constexpr std::int32_t EXPLOSION_LAYER = 1;

  /** @brief A server entity and its local copy. */
  struct Mirrored {
    /** @brief The local entity. */
    ampersand::core::ecs::Entity entity;
    /** @brief Its kind, to know if it explodes. */
    common::EntityKind kind;
  };

  /**
   * @brief Look of a new entity: by its kind, and by its owner for a ship.
   *
   * @param packet The ENTITY_SPAWN received.
   * @return The sprite and animation to give it.
   */
  [[nodiscard]] const Look& lookFor(const protocol::EntitySpawn& packet) const;

  /**
   * @brief Creates a local entity with a look.
   *
   * @param look Its sprite, scale and animation.
   * @param position Its position, in world units.
   * @return The entity, or nothing if the world is full.
   */
  std::optional<ampersand::core::ecs::Entity> create(
      const Look& look, ampersand::core::math::Vec2f position);

  /**
   * @brief Creates an explosion centered on the sprite of an entity that
   *        died, and plays its sound.
   *
   * @param transform Transform of the entity that died.
   * @param sprite Sprite of the entity that died.
   */
  void explode(const ampersand::core::Transform& transform,
               const ampersand::render::Sprite& sprite);

  /**
   * @brief Plays a sound on an entity. Does nothing if the sound was not
   *        loaded.
   *
   * @param entity The entity playing it.
   * @param sound The sound, 0 if it was not loaded.
   * @param volume Its volume, from 0 to 100.
   */
  void play(ampersand::core::ecs::Entity entity,
            ampersand::audio::SoundId sound, float volume);

  /** @brief The world holding the local entities. */
  ampersand::core::ecs::World& _world;
  /** @brief Sprites and sounds of each kind of entity. */
  EntityLooks _looks;
  /** @brief The mirrored entities, by server id. */
  std::unordered_map<std::uint32_t, Mirrored> _entities;
  /** @brief Ids destroyed: ids are never reused, so a spawn arriving after
   *         its destroy is dropped. */
  std::unordered_set<std::uint32_t> _destroyed;
  /** @brief Server tick of the last snapshot applied. */
  std::uint32_t _lastSnapshotTick{0};
};

}  // namespace rtype::client
