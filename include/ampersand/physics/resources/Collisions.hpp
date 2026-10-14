#pragma once

#include <ampersand/core/ecs/Entity.hpp>
#include <utility>
#include <vector>

namespace ampersand::physics {

/**
 * @brief Pairs of entities whose colliders overlap, refilled by the
 * CollisionSystem every frame (the order inside a pair is not defined).
 */
struct Collisions {
    std::vector<std::pair<core::ecs::Entity, core::ecs::Entity>> pairs;
};

}  // namespace ampersand::physics
