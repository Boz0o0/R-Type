#pragma once

namespace ampersand::physics {

/**
 * @brief Strength of the gravity for the whole World, multiplied by the scale
 * of each Gravity component.
 */
struct GravityForce {
    /** @brief Speed gained each second, in units per second
     * (positive goes down). */
    float value{};
};

}  // namespace ampersand::physics
