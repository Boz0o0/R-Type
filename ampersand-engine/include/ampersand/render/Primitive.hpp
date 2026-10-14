#pragma once

#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/render/Color.hpp>

namespace ampersand::render {

/**
 * @brief Filled rectangle with an optional outline.
 */
struct Rectangle {
    core::math::Vec2f size;
    Color fill{};
    Color outline{};
    float outlineThickness{};
};

/**
 * @brief Filled circle with an optional outline.
 */
struct Circle {
    float radius{};
    Color fill{};
    Color outline{};
    float outlineThickness{};
};

/**
 * @brief A line segment.
 */
struct Line {
    core::math::Vec2f start;
    core::math::Vec2f end;
    Color color{};
};

}  // namespace ampersand::render
