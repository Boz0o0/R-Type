#pragma once

#include <ampersand/core/math/Rect.hpp>
#include <ampersand/render/Color.hpp>
#include <ampersand/render/TextureId.hpp>
#include <cstdint>

namespace ampersand::render {

/**
 * @brief Image drawn at the entity's Transform by the RenderSystem.
 */
struct Sprite {
    TextureId texture{};
    /** @brief Region of the texture to draw, in pixels (empty = whole
     * texture). */
    core::math::Rect<std::int32_t> source;
    /** @brief Tint multiplied with the texture (white = original image). */
    Color color{.r = 255, .g = 255, .b = 255};
};

}  // namespace ampersand::render
