#pragma once

#include <rtype/common/components/Kind.hpp>

namespace rtype::common {

/**
 * @brief Gun of an entity.
 *
 * While firing is true, the ShootingSystem fires a missile every cooldown
 * seconds. Whoever holds the trigger sets firing: always true for an enemy,
 * the shoot key for a player.
 */
struct Weapon {
  /** @brief Seconds between two shots. */
  float cooldown{};
  /** @brief Seconds before the next shot. */
  float remaining{};
  /** @brief Whether the trigger is held. */
  bool firing{};
  /** @brief Missile it fires (see createMissile). */
  EntityKind missile{};
};

}  // namespace rtype::common
