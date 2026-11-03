#pragma once

#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/render/Color.hpp>
#include <ampersand/render/FontId.hpp>
#include <cstdint>

namespace rtype::client {

/** @brief Layer of the HUD and the menus, over the game. */
inline constexpr std::int32_t TEXT_LAYER = 10;

/**
 * @brief Creates an entity that draws a text, empty for now, over the game.
 *
 * @param world The world to create the entity in.
 * @param font Font of the text.
 * @param position Top left corner of the text, in world units.
 * @param size Character size.
 * @param color Color of the text, white by default.
 * @return The entity, with a Transform and a Text on TEXT_LAYER.
 */
ampersand::core::ecs::Entity createText(
    ampersand::core::ecs::World& world, ampersand::render::FontId font,
    ampersand::core::math::Vec2f position, unsigned int size,
    ampersand::render::Color color = {.r = 255, .g = 255, .b = 255});

}  // namespace rtype::client
