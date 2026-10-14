#pragma once

#include <cstdint>

namespace ampersand::core::math {

template <typename T>
struct Vec2 {
    T x{};
    T y{};

    constexpr Vec2() = default;
    constexpr Vec2(T xValue, T yValue) : x(xValue), y(yValue) {}
};

using Vec2f = Vec2<float>;
using Vec2i = Vec2<int>;
using Vec2u = Vec2<std::uint32_t>;

}  // namespace ampersand::core::math