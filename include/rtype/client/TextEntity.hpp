#pragma once

#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/render/Color.hpp>
#include <ampersand/render/FontId.hpp>

namespace rtype::client {

// Layer of the HUD and the menus, over the game.
inline constexpr float TEXT_LAYER = 10.0F;

// Creates an entity that draws a text, empty for now, over the game.
ampersand::core::ecs::Entity createText(
    ampersand::core::ecs::World& world, ampersand::render::FontId font,
    ampersand::core::math::Vec2f position, unsigned int size,
    ampersand::render::Color color = {.r = 255, .g = 255, .b = 255});

}  // namespace rtype::client
