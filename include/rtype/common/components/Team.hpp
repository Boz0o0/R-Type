#pragma once

#include <cstdint>

namespace rtype::common {

/** @brief Side of an entity in the fight. */
enum class Side : std::uint8_t {
  /** @brief The ships of the players and their missiles. */
  Players,
  /** @brief The enemies and their missiles. */
  Enemies,
};

/**
 * @brief Side of an entity: its Damage does not hurt the entities of the same
 *        side.
 *
 * Give it to a ship or an enemy and to each of its missiles. An entity without
 * a Team (a destroyable tile) hurts and is hurt by every side.
 */
struct Team {
  /** @brief Side of the entity. */
  Side side{};
};

}  // namespace rtype::common
