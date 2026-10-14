#pragma once

#include <ampersand/core/math/Rect.hpp>
#include <cstdint>
#include <vector>

namespace ampersand::render {

/**
 * @brief Plays a list of frames on the entity's Sprite, driven by the
 * AnimationSystem.
 *
 * Each frame is a region of the Sprite's texture (like Sprite::source). The
 * AnimationSystem writes the current frame into Sprite::source. Set
 * `elapsed` back to 0 to restart from the first frame.
 */
struct Animation {
    std::vector<core::math::Rect<std::int32_t>> frames;
    /** @brief Seconds each frame stays on screen (must be positive). */
    float frameDuration{0.1F};
    /** @brief Starts over after the last frame, or stays on it if false. */
    bool loop{true};
    /** @brief Seconds since the first frame, advanced by the
     * AnimationSystem. */
    float elapsed{};
};

/**
 * @brief Checks if a non-looping Animation reached its last frame
 *
 * @param animation Animation to check.
 * @return true if `loop` is false and the whole animation was played. A
 * looping animation never finishes.
 */
[[nodiscard]] bool isFinished(const Animation& animation);

}  // namespace ampersand::render
