#pragma once

#include <ampersand/core/math/Vec2.hpp>

namespace ampersand::physics {

/**
 * @brief Speed of an entity (x and y, no z).
 */
struct Velocity {
    core::math::Vec2f value;
};

}  // namespace ampersand::physics
