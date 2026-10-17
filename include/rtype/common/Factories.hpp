#pragma once

#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <rtype/common/components/Kind.hpp>

namespace rtype::common {

/**
 * @brief Creates a missile fired by an entity.
 *
 * The missile starts at the position of the shooter, with its Team (if any).
 * Its kind gives the rest (speed, damage, sprite, etc.).
 *
 * @param world World holding the shooter.
 * @param kind Kind of the missile.
 * @param shooter Entity that fires, with a Transform.
 * @return The missile.
 * @throws std::invalid_argument if kind is not a missile.
 */
ampersand::core::ecs::Entity createMissile(
    ampersand::core::ecs::World& world, EntityKind kind,
    ampersand::core::ecs::Entity shooter);

}  // namespace rtype::common
