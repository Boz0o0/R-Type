#pragma once

#include <ampersand/core/math/Vec2.hpp>
#include <cstdint>

namespace ampersand::input {

/**
 * @brief Enumeration of supported keyboard keys.
 *
 * Letters follow the layout of the keyboard: on AZERTY, the key at the place
 * of the QWERTY W is Z.
 */
enum class Key : std::uint8_t {
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    Num0,
    Num1,
    Num2,
    Num3,
    Num4,
    Num5,
    Num6,
    Num7,
    Num8,
    Num9,
    Up,
    Down,
    Left,
    Right,
    Space,
    Enter,
    Escape,
    Tab,
    Backspace,
    LShift,
    RShift,
    LControl,
    RControl,
    LAlt,
    RAlt,
    Unknown,
};

/**
 * @brief Enumeration of supported mouse buttons.
 */
enum class Button : std::uint8_t { Left, Right, Middle, Unknown };

/**
 * @brief Event triggered when a key is pressed.
 */
struct KeyPressed {
    Key key;
};

/**
 * @brief Event triggered when a key is released.
 */
struct KeyReleased {
    Key key;
};

/**
 * @brief Event triggered when a mouse button is pressed.
 */
struct MousePressed {
    Button button;
    /** @brief Position of the mouse in the window, in pixels. */
    core::math::Vec2i position;
};

/**
 * @brief Event triggered when a mouse button is released.
 */
struct MouseReleased {
    Button button;
    /** @brief Position of the mouse in the window, in pixels. */
    core::math::Vec2i position;
};

/**
 * @brief Event triggered when the mouse moves inside the window.
 */
struct MouseMoved {
    /** @brief Position of the mouse in the window, in pixels. */
    core::math::Vec2i position;
};

}  // namespace ampersand::input
