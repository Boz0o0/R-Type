#pragma once

#include <cstdint>

namespace ampersand::input {

/**
 * @brief Enumeration of supported keyboard keys.
 */
enum class Key : std::uint8_t { Up, Down, Left, Right, Space, Escape, Unknown };

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

}  // namespace ampersand::input
