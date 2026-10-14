#pragma once

#include <ampersand/render/Color.hpp>

namespace ampersand::render {

/**
 * @brief Color the window is filled with before each frame is drawn.
 *
 * Optional: without it, the RenderSystem clears in black.
 */
struct ClearColor {
    Color value;
};

}  // namespace ampersand::render
