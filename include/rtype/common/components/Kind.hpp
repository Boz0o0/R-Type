#pragma once

#include <cstdint>

namespace rtype::common {

/**
 * @brief What an entity is.
 */
enum class EntityKind : std::uint8_t {
  /** @brief Ship of a player. */
  Ship = 0,
  /** @brief Enemy. */
  Enemy = 1,
  /** @brief Missile fired by a player. */
  PlayerMissile = 2,
  /** @brief Missile fired by an enemy. */
  EnemyMissile = 3,
  /** @brief Bonus a player can pick up. */
  PowerUp = 4,
  /** @brief Part of the level, like a wall. */
  Obstacle = 5,
};

/**
 * @brief What an entity is: the server sends it to the clients so they know
 *        what to draw.
 */
struct Kind {
  /** @brief What the entity is. */
  EntityKind kind{};
};

}  // namespace rtype::common
