#pragma once

#include <ampersand/core/math/Vec2.hpp>
#include <cstdint>
#include <rtype/common/components/Kind.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <rtype/common/resources/Deaths.hpp>
#include <variant>

/**
 * @file GameEvents.hpp
 * @brief What the Game tells the network layer after each tick.
 *
 * The Game never touches the network: GameServer turns these events into
 * packets for the clients.
 */

namespace rtype::server {

/** @brief What a client needs to know to draw an entity. */
struct EntityInfo {
  /** @brief Id given by the Game, never reused while the server runs. */
  std::uint32_t id{};
  common::EntityKind kind{};
  /** @brief Player that owns it, or protocol::NO_OWNER. */
  std::uint8_t ownerId{protocol::NO_OWNER};
  /** @brief Top-left corner, in world units. */
  ampersand::core::math::Vec2f position;
  /** @brief World units per second. */
  ampersand::core::math::Vec2f velocity;
};

/** @brief An entity appeared (ship, enemy, missile...). */
struct EntitySpawned {
  EntityInfo entity;
};

/** @brief An entity left the world. */
struct EntityDestroyed {
  std::uint32_t id{};
  common::DeathReason reason{};
};

/** @brief Health, lives or score of a player changed. */
struct PlayerChanged {
  std::uint8_t playerId{};
  /** @brief Hit points of the ship, 0 while it has none. */
  std::uint8_t health{};
  std::uint8_t lives{};
  std::uint32_t score{};

  bool operator==(const PlayerChanged&) const = default;
};

/** @brief A new game begins: the ships of the players are spawned. */
struct GameStarted {};

/** @brief The game is over; a new one starts after Game::RESTART_DELAY. */
struct GameEnded {
  bool victory{};
  /** @brief Sum of the scores of the players. */
  std::uint32_t score{};
};

/** @brief Anything that happened during a tick. */
using GameEvent = std::variant<EntitySpawned, EntityDestroyed, PlayerChanged,
                               GameStarted, GameEnded>;

}  // namespace rtype::server
