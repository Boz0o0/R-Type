#pragma once

#include <ampersand/core/math/Vec3.hpp>

namespace ampersand::core {

/**
 * @brief Position, scale, rotation, and origin for a 3D object.
 */
struct Transform {
    math::Vec3f position;
    math::Vec3f scale{1.0F, 1.0F, 1.0F};
    math::Vec3f rotationDegrees;
    math::Vec3f origin;
};

}  // namespace ampersand::core
