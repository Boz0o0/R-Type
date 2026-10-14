#pragma once

#include <ampersand/core/math/Vec2.hpp>

namespace ampersand::render {

/**
 * @brief Part of the world shown in the window, in world coordinates.
 *
 * Optional: without it, the RenderSystem uses the default view (window
 * pixels, (0, 0) at the top-left).
 */
struct Camera {
    /** @brief World point shown at the middle of the window. */
    core::math::Vec2f center;
    /** @brief Width and height of the world area shown (must be positive). */
    core::math::Vec2f size;
};

}  // namespace ampersand::render
