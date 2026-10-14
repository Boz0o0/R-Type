#pragma once

#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/input/Event.hpp>
#include <variant>

namespace ampersand::render {

/**
 * @brief Event triggered when the user closes the application window.
 */
struct Closed {};

/**
 * @brief Event triggered when the window size changes.
 */
struct Resized {
    ampersand::core::math::Vec2u size;
};

/**
 * @brief Union of all supported rendering and input events.
 */
using Event =
    std::variant<Closed, Resized, input::KeyPressed, input::KeyReleased>;

}  // namespace ampersand::render