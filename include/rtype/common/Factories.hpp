#pragma once

#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/math/Vec2.hpp>
#include <optional>
#include <rtype/common/components/Kind.hpp>

namespace rtype::common {

/**
 * @brief Creates the ship of a player, at rest, not firing.
 *
 * Health 3, Team Players, hitbox 64x32, Weapon firing a PlayerMissile every
 * 0.25 s. Added to the Spawns.
 *
 * @param world World to create it in.
 * @param position Top-left corner of the ship.
 * @return The ship, or std::nullopt if the World is full.
 * @throws ResourceNotFound if the World has no Spawns.
 */
std::optional<ampersand::core::ecs::Entity> createShip(
    ampersand::core::ecs::World& world,
    const ampersand::core::math::Vec2f& position);

/**
 * @brief Creates an enemy, flying to the left and firing.
 *
 * Health 2, Damage 1, Team Enemies, hitbox 64x64, velocity 200 to the left,
 * Weapon firing an EnemyMissile every 1.5 s (the first one 1.5 s after it
 * appears). Added to the Spawns.
 *
 * @param world World to create it in.
 * @param position Top-left corner of the enemy.
 * @return The enemy, or std::nullopt if the World is full.
 * @throws ResourceNotFound if the World has no Spawns.
 */
std::optional<ampersand::core::ecs::Entity> createEnemy(
    ampersand::core::ecs::World& world,
    const ampersand::core::math::Vec2f& position);

/**
 * @brief Creates a missile fired by an entity.
 *
 * The missile starts at the position of the shooter, with its Team (if any).
 * Its kind gives the rest (speed, damage, sprite, etc.).
 *
 * @param world World holding the shooter.
 * @param kind Kind of the missile.
 * @param shooter Entity that fires, with a Transform.
 * @return The missile, or std::nullopt if the World is full.
 * @throws std::invalid_argument if kind is not a missile.
 * @throws ResourceNotFound if the World has no Spawns.
 */
std::optional<ampersand::core::ecs::Entity> createMissile(
    ampersand::core::ecs::World& world, EntityKind kind,
    ampersand::core::ecs::Entity shooter);

}  // namespace rtype::common
