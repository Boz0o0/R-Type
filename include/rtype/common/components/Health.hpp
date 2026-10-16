#pragma once

#include <cstdint>

namespace rtype::common {

/**
 * @brief Hit points of an entity that can be hurt.
 */
struct Health {
  /** @brief Hit points left. */
  std::int32_t current{};
  /** @brief Hit points when the entity is created. */
  std::int32_t max{};
};

}  // namespace rtype::common
