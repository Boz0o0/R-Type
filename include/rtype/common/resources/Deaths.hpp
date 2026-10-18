#pragma once

#include <ampersand/core/ecs/Entity.hpp>
#include <cstdint>
#include <optional>
#include <vector>

namespace rtype::common {

/**
 * @brief Why an entity died.
 */
enum class DeathReason : std::uint8_t {
  /** @brief Its Health reached 0, or it is a missile that hit something. */
  Killed = 0,
  /** @brief It left the screen. */
  OutOfScreen = 1,
  /** @brief A player picked it up (collectible). */
  Collected = 2,
};

/** @brief One entity that died and why */
struct Death {
  /** @brief Entity that died. */
  ampersand::core::ecs::Entity entity;
  /** @brief Why it died. */
  DeathReason reason{};
  /**
   * @brief Entity whose Damage removed its last hit points, when it was
   *        Killed by one (a missile, an enemy...). Still alive until the
   *        CleanupSystem, so the server can credit its Owner.
   */
  std::optional<ampersand::core::ecs::Entity> killer;
};

/**
 * @brief Entities that died during the tick.
 *
 * They stay alive until the CleanupSystem, which runs last, destroys them and
 * empties the list: until then, the server can read why each one died.
 */
struct Deaths {
  /** @brief Deaths of the tick. */
  std::vector<Death> entities;
};

}  // namespace rtype::common
