#pragma once

#include <string>

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
  /** @brief Prefab of the missile it fires. */
  std::string missile;
};

}  // namespace rtype::common
