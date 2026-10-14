#pragma once

namespace ampersand::physics {

/**
 * @brief Makes an entity with a Velocity fall, using the GravityForce
 * resource (an entity without Gravity does not fall).
 */
struct Gravity {
    /** @brief 1 falls, -1 goes up, 0 does nothing, 2 falls twice as fast. */
    float scale{1.0F};
};

}  // namespace ampersand::physics
