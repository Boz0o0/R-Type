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
   * @param factory Creates each kind of entity with its look and sounds.
   */
  WorldMirror(ampersand::core::ecs::World& world, IEntityFactory& factory);

  /**
   * @brief Tells which ship is ours: it is predicted (moved from our input)
   *        instead of following the snapshots. Each room gives its own id.
   *
   * @param playerId Our id in the room, or std::nullopt out of a room.
   */
  void setLocalPlayer(std::optional<std::uint8_t> playerId);

  /**
   * @brief Creates the local entity of a server entity. Ignored if it
   *        already exists, was already destroyed, or the world is full.
   *
   * @param packet The ENTITY_SPAWN received.
   */
  void spawn(const protocol::EntitySpawn& packet);

  /**
   * @brief Gives the known entities their velocity in the snapshot, and the
   *        gap to their position as a Correction, or moves them there if the
   *        gap is over SNAP_DISTANCE. Our predicted ship only keeps the server
   *        state. Ignored if it is older than the last one applied.
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
  /** @brief Bigger gaps are jumped (several lost snapshots): smoothing them
   *         would show the entity flying across the screen. */
  static constexpr float SNAP_DISTANCE = 128.0F;

  /** @brief A server entity and its local copy. */
  struct Mirrored {
    /** @brief The local entity. */
    ampersand::core::ecs::Entity entity;
    /** @brief Its kind, to know if it explodes. */
    common::EntityKind kind;
  };

  /** @brief Whether the ENTITY_SPAWN is our own ship. */
  [[nodiscard]] bool isLocalShip(const protocol::EntitySpawn& packet) const;

  /**
   * @brief Sets the Correction of an entity toward the server position, or
   *        moves it there at once if the gap is over SNAP_DISTANCE.
   *
   * @param entity A mirrored entity, not predicted.
   * @param serverPosition Its position in the snapshot.
   */
  void correct(ampersand::core::ecs::Entity entity,
               ampersand::core::math::Vec2f serverPosition);

  /**
   * @brief Creates an explosion centered on the sprite of an entity that
   *        died.
   *
   * @param transform Transform of the entity that died.
   * @param sprite Sprite of the entity that died.
   */
  void explode(const ampersand::core::Transform& transform,
               const ampersand::render::Sprite& sprite);

  /** @brief The world holding the local entities. */
  ampersand::core::ecs::World& _world;
  /** @brief Creates each kind of entity with its look and sounds. */
  IEntityFactory& _factory;
  /** @brief The mirrored entities, by server id. */
  std::unordered_map<std::uint32_t, Mirrored> _entities;
  /** @brief Ids destroyed: ids are never reused, so a spawn arriving after
   *         its destroy is dropped. */
  std::unordered_set<std::uint32_t> _destroyed;
  /** @brief Server tick of the last snapshot applied. */
  std::uint32_t _lastSnapshotTick{0};
  /** @brief Our id in the room: its ship is the predicted one. */
  std::optional<std::uint8_t> _localPlayer;
};

}  // namespace rtype::client
