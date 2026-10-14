#pragma once

namespace ampersand::core {

/**
 * @brief Time of the game loop, filled by the game every frame.
 */
struct Clock {
    /** @brief Seconds since the previous frame. */
    float dt{};
    /** @brief Seconds since the start of the game. */
    float elapsed{};
};

}  // namespace ampersand::core
