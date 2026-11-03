#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/render/Color.hpp>
#include <ampersand/render/FontId.hpp>
#include <ampersand/render/components/Text.hpp>
#include <rtype/client/TextEntity.hpp>

namespace rtype::client {

namespace core = ampersand::core;
namespace render = ampersand::render;

core::ecs::Entity createText(core::ecs::World& world, render::FontId font,
                             core::math::Vec2f position, unsigned int size,
                             render::Color color) {
  const core::ecs::Entity entity = world.create();
  world.add(entity, core::Transform{
                        .position = {position.x, position.y, 0.0F},
                    });
  world.add(entity, render::Text{
                        .font = font,
                        .value = {},
                        .size = size,
                        .color = color,
                        .layer = TEXT_LAYER,
                    });
  return entity;
}

}  // namespace rtype::client
