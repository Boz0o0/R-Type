#pragma once

#include <ampersand/scene/resources/SceneManager.hpp>

namespace rtype::common {

/**
 * @brief Lets the prefabs and scenes give the components of the game.
 *
 * Registers Kind, Health, Damage, Team, Weapon and Projectile. 
 * Owner is not registered: a prefab cannot know the player, so the
 * server adds it when it creates the ship of a player.
 *
 * @param scenes SceneManager that loads the files, before loadConfig.
 */
void registerComponents(ampersand::scene::SceneManager& scenes);

}  // namespace rtype::common
