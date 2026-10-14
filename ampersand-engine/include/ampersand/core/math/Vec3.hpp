#pragma once

namespace ampersand::core::math {

template <typename T>
struct Vec3 {
    T x{};
    T y{};
    T z{};

    constexpr Vec3() = default;
    constexpr Vec3(T xValue, T yValue, T zValue)
        : x(xValue), y(yValue), z(zValue) {}
};

using Vec3f = Vec3<float>;
using Vec3i = Vec3<int>;

}  // namespace ampersand::core::math
