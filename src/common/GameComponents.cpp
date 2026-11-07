#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/scene/resources/SceneManager.hpp>
#include <nlohmann/json.hpp>
#include <rtype/common/GameComponents.hpp>
#include <rtype/common/components/Damage.hpp>
#include <rtype/common/components/Health.hpp>
#include <rtype/common/components/Kind.hpp>
#include <rtype/common/components/Projectile.hpp>
#include <rtype/common/components/Team.hpp>
#include <rtype/common/components/Weapon.hpp>

namespace rtype::common {

// NOLINTBEGIN(readability-identifier-length,modernize-use-constraints)
NLOHMANN_JSON_SERIALIZE_ENUM(EntityKind,
                             {
                                 {EntityKind::Ship, "Ship"},
                                 {EntityKind::Enemy, "Enemy"},
                                 {EntityKind::PlayerMissile, "PlayerMissile"},
                                 {EntityKind::EnemyMissile, "EnemyMissile"},
                                 {EntityKind::PowerUp, "PowerUp"},
                                 {EntityKind::Obstacle, "Obstacle"},
                             })
NLOHMANN_JSON_SERIALIZE_ENUM(Side, {
                                       {Side::Players, "Players"},
                                       {Side::Enemies, "Enemies"},
                                   })
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Kind, kind, variant)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Health, current, max)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Damage, amount)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Team, side)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Weapon, cooldown, remaining, firing, missile)
// NOLINTEND(readability-identifier-length,modernize-use-constraints)

void registerComponents(ampersand::scene::SceneManager& scenes) {
  scenes.registerComponent<Kind>("Kind");
  scenes.registerComponent<Health>("Health");
  scenes.registerComponent<Damage>("Damage");
  scenes.registerComponent<Team>("Team");
  scenes.registerComponent<Weapon>("Weapon");
  scenes.registerComponent("Projectile",
                           [](ampersand::core::ecs::World& world,
                              ampersand::core::ecs::Entity entity,
                              const ampersand::scene::json& /*data*/) {
                             world.add(entity, Projectile{});
                           });
}

}  // namespace rtype::common
