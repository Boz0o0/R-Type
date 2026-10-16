#pragma once

#include <cstdint>

namespace rtype::common {

/**
 * @brief Hit points removed from each entity with a Health that this entity
 *        touches.
 *
 * An entity with a Damage but no Health (a missile) dies on its first hit.
 */
struct Damage {
  /** @brief Hit points removed per hit. */
  std::int32_t amount{};
};

}  // namespace rtype::common
