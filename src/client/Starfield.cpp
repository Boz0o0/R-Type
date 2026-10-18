#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/physics/components/Velocity.hpp>
#include <ampersand/render/TextureId.hpp>
#include <ampersand/render/components/Sprite.hpp>
#include <cstddef>
#include <rtype/client/Starfield.hpp>
#include <rtype/client/components/Scrolling.hpp>

namespace rtype::client {

namespace core = ampersand::core;
namespace physics = ampersand::physics;
namespace render = ampersand::render;

void Starfield::spawn(core::ecs::World& world, const Textures& textures) {
  for (std::size_t index = 0; index < LAYER_COUNT; ++index) {
    addCopy(world, textures[index], LAYERS[index], 0.0F);
    addCopy(world, textures[index], LAYERS[index], WIDTH);
  }
}

void Starfield::addCopy(core::ecs::World& world, render::TextureId texture,
                        const Layer& layer, float positionX) {
  const core::ecs::Entity entity = world.create();
  world.add(entity, core::Transform{
                        .position = {positionX, 0.0F, layer.depth},
                        .scale = {SCALE, SCALE, 1.0F},
                    });
  world.add(entity, render::Sprite{.texture = texture});
  world.add(entity, physics::Velocity{.value = {-layer.speed, 0.0F}});
  world.add(entity, Scrolling{.width = WIDTH});
}

}  // namespace rtype::client
