#pragma once

#include <cstdint>

namespace ampersand::core::math {

template <typename T>
struct Rect {
    T x{};
    T y{};
    T width{};
    T height{};

    constexpr Rect() = default;

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    constexpr Rect(T xValue, T yValue, T widthValue, T heightValue)
        : x(xValue), y(yValue), width(widthValue), height(heightValue) {}

    /**
     * @brief Checks if two rectangles share at least one point (rectangles
     * that only touch by an edge do not intersect).
     *
     * @param other Rectangle to test against.
     * @return true if the rectangles overlap.
     */
    [[nodiscard]] constexpr bool intersects(const Rect& other) const {
        return x < other.x + other.width && other.x < x + width &&
               y < other.y + other.height && other.y < y + height;
    }
};

}  // namespace ampersand::core::math