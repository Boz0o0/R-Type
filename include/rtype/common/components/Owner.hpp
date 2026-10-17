#pragma once

#include <cstdint>

namespace rtype::common {

/**
 * @brief Player an entity belongs to: their ship, and every missile it fires.
 *
 * createMissile copies it from the shooter, so a kill can be credited to the
 * player who fired.
 */
struct Owner {
  /** @brief Id of the player, from 0 to MAX_PLAYERS - 1. */
  std::uint8_t playerId{};
};

}  // namespace rtype::common
