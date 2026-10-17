#pragma once

#include <ampersand/core/ecs/Entity.hpp>
#include <vector>

namespace rtype::common {

/**
 * @brief Entities created during the tick.
 *
 * Every factory adds the entity it creates. The server reads it to send the
 * new entities to the clients; the CleanupSystem empties it at the end of the
 * tick.
 */
struct Spawns {
  /** @brief Entities created this tick, in order. */
  std::vector<ampersand::core::ecs::Entity> entities;
};

}  // namespace rtype::common
