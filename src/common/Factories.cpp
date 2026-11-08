#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/core/math/Vec3.hpp>
#include <ampersand/physics/components/BoxCollider.hpp>
#include <ampersand/physics/components/Velocity.hpp>
#include <ampersand/scene/resources/SceneManager.hpp>
#include <optional>
#include <rtype/common/Factories.hpp>
#include <rtype/common/components/Owner.hpp>
#include <rtype/common/components/Team.hpp>
#include <stdexcept>
#include <string>

namespace rtype::common {

using ampersand::core::ecs::Entity;
using ampersand::core::ecs::World;
using ampersand::physics::BoxCollider;
using ampersand::physics::Velocity;
using ampersand::scene::SceneManager;

namespace {

std::optional<Entity> instantiate(
    World& world, SceneManager& scenes, const std::string& prefab,
    const ampersand::core::math::Vec3f& position) {
  Entity entity;
  try {
    entity = scenes.instantiate(world, prefab);
  } catch (const std::runtime_error&) {
    return std::nullopt;
  }
  world.get<ampersand::core::Transform>(entity).position = position;
  return entity;
}

}  // namespace

std::optional<Entity> createShip(World& world, SceneManager& scenes,
                                 const ampersand::core::math::Vec2f& position) {
  return instantiate(world, scenes, std::string(SHIP_PREFAB),
                     {position.x, position.y, 0.0F});
}

std::optional<Entity> createMissile(World& world, SceneManager& scenes,
                                    const std::string& prefab, Entity shooter) {
  const ampersand::core::math::Vec3f position =
      world.get<ampersand::core::Transform>(shooter).position;
  const auto missile = instantiate(world, scenes, prefab, position);
  if (!missile) {
    return std::nullopt;
  }
  if (world.has<BoxCollider>(shooter) && world.has<BoxCollider>(*missile) &&
      world.has<Velocity>(*missile)) {
    const auto& gun = world.get<BoxCollider>(shooter).bounds;
    const auto& bullet = world.get<BoxCollider>(*missile).bounds;
    auto& start = world.get<ampersand::core::Transform>(*missile).position;
    start.x += world.get<Velocity>(*missile).value.x < 0.0F ? -bullet.width
                                                            : gun.width;
    start.y += (gun.height - bullet.height) / 2.0F;
  }
  if (world.has<Team>(shooter)) {
    world.add(*missile, world.get<Team>(shooter));
  }
  if (world.has<Owner>(shooter)) {
    world.add(*missile, world.get<Owner>(shooter));
  }
  return missile;
}

}  // namespace rtype::common
