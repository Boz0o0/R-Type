#pragma once

#include <cstdint>

namespace rtype::common {

/** @brief What an entity is. */
enum class EntityKind : std::uint8_t {
  /** @brief Ship of a player. */
  Ship,
  /** @brief Enemy. */
  Enemy,
  /** @brief Missile fired by a player. */
  PlayerMissile,
  /** @brief Missile fired by an enemy. */
  EnemyMissile,
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
