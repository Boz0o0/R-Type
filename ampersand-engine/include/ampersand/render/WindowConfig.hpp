#pragma once

#include <ampersand/core/math/Vec2.hpp>
#include <string>

namespace ampersand::render {

/**
 * @brief Configuration used to initialize a window instance.
 */
struct WindowConfig {
    ampersand::core::math::Vec2u size;
    std::string title;
    bool vsync{true};
};

}  // namespace ampersand::render
