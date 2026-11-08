#pragma once

#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/scene/resources/SceneManager.hpp>
#include <optional>
#include <string>
#include <string_view>

namespace rtype::common {

/** @brief Prefab of the ship of a player, relative to the game folder. */
inline constexpr std::string_view SHIP_PREFAB =
    "world/prefabs/common/ship.json";

/**
 * @brief Creates the ship of a player from SHIP_PREFAB, at rest, not firing.
 *
 * @param world World to create it in.
 * @param scenes SceneManager whose configuration gives the game folder.
 * @param position Top-left corner of the ship.
 * @return The ship, or std::nullopt if the World is full or the prefab
 *         cannot be loaded.
 */
std::optional<ampersand::core::ecs::Entity> createShip(
    ampersand::core::ecs::World& world, ampersand::scene::SceneManager& scenes,
    const ampersand::core::math::Vec2f& position);

/**
 * @brief Creates a missile fired by an entity.
 *
 * The missile starts in front of the shooter, centered on its height: on
 * its right if it flies right, on its left otherwise (from their hitboxes).
 * It gets the Team and the Owner (if any) of the shooter. Its prefab gives
 * the rest (kind, speed, damage, hitbox).
 *
 * @param world World holding the shooter.
 * @param scenes SceneManager whose configuration gives the game folder.
 * @param prefab Prefab of the missile, relative to the game folder.
 * @param shooter Entity that fires, with a Transform.
 * @return The missile, or std::nullopt if the World is full or the prefab
 *         cannot be loaded.
 */
std::optional<ampersand::core::ecs::Entity> createMissile(
    ampersand::core::ecs::World& world, ampersand::scene::SceneManager& scenes,
    const std::string& prefab, ampersand::core::ecs::Entity shooter);

}  // namespace rtype::common
