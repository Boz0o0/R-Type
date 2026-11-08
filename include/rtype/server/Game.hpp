#pragma once

#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/physics/systems/CollisionSystem.hpp>
#include <ampersand/physics/systems/MovementSystem.hpp>
#include <ampersand/scene/resources/SceneManager.hpp>
#include <ampersand/scripting/systems/ScriptingSystem.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <random>
#include <rtype/common/ShipMovement.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <rtype/common/resources/Deaths.hpp>
#include <rtype/common/systems/CleanupSystem.hpp>
#include <rtype/common/systems/DamageSystem.hpp>
#include <rtype/common/systems/OutOfScreenSystem.hpp>
#include <rtype/common/systems/ShootingSystem.hpp>
#include <rtype/server/GameEvents.hpp>
#include <string_view>
#include <vector>

namespace rtype::server {

/**
 * @brief Id of an entity for the clients, given when the Game announces it
 *        with EntitySpawned.
 */
struct NetworkId {
  std::uint32_t value{};
};

/**
 * @brief The game logic of the server: the World, its systems and the rules.
 *
 * It knows nothing about the network: GameServer calls addPlayer(),
 * setInput()... from the packets it receives, and sends to the clients the
 * events returned by update().
 *
 * A game goes Waiting (no player) -> Playing (waves of Bydos) -> Over (every
 * ship lost, or every wave beaten) -> Playing again after RESTART_DELAY.
 *
 * The entities come from the prefabs of game/world/prefabs/common, read from
 * the working directory (the repository, or the binary folder of the client).
 * The waves come from the scripts of the server scene: they create the
 * enemies with instantiate(), and call finishWaves() after the last wave.
 */
class Game {
 public:
  enum class Phase : std::uint8_t {
    /** @brief No player: nothing happens. */
    Waiting,
    /** @brief The waves are coming. */
    Playing,
    /** @brief GameEnded was sent; the next game starts soon. */
    Over,
  };

  /** @brief Size of the screen, in world units (shared with the client). */
  static constexpr float SCREEN_WIDTH = common::ShipMovement::SCREEN.x;
  static constexpr float SCREEN_HEIGHT = common::ShipMovement::SCREEN.y;
  /** @brief World units per second of a ship. */
  static constexpr float SHIP_SPEED = common::ShipMovement::SPEED;
  /** @brief x of a ship when it appears. */
  static constexpr float SHIP_START_X = 128.0F;
  /** @brief Ships of a player in a game. */
  static constexpr std::uint8_t STARTING_LIVES = 3;
  /** @brief Seconds before a lost ship comes back. */
  static constexpr float RESPAWN_DELAY = 2.0F;
  /** @brief Seconds between GameEnded and the next GameStarted. */
  static constexpr float RESTART_DELAY = 5.0F;
  /** @brief Score given to the player whose missile kills an enemy. */
  static constexpr std::uint32_t ENEMY_SCORE = 100;
  /** @brief Configuration of the server, from the working directory. */
  static constexpr std::string_view CONFIG = "game/world/server.json";

  /**
   * @brief Creates a game with no player.
   *
   * @param seed Seed of math.random in the scripts (heights of the enemies).
   * @throws std::runtime_error if CONFIG cannot be loaded.
   */
  explicit Game(std::uint32_t seed = std::random_device{}());

  /**
   * @brief Adds a player. Its ship appears on the next update(), or when the
   *        next game starts.
   *
   * @param playerId Id of the player, below MAX_PLAYERS, not in the game.
   */
  void addPlayer(std::uint8_t playerId);

  /**
   * @brief Removes a player: its ship is destroyed on the next update().
   *
   * @param playerId Id of the player; nothing happens if it is not in the
   *        game.
   */
  void removePlayer(std::uint8_t playerId);

  /**
   * @brief Sets the keys a player holds (InputFlag bits), used by every
   *        update() until the next call.
   */
  void setInput(std::uint8_t playerId, std::uint8_t inputMask);

  /**
   * @brief Runs one tick of the game.
   *
   * @param deltaTime Seconds since the previous tick.
   * @return What happened, in order.
   */
  [[nodiscard]] std::vector<GameEvent> update(float deltaTime);

  /** @brief Every entity already announced, as it is now. */
  [[nodiscard]] std::vector<EntityInfo> entities();

  /** @brief Health, lives and score of every player. */
  [[nodiscard]] std::vector<PlayerChanged> playerStates();

  [[nodiscard]] Phase phase() const noexcept;
  [[nodiscard]] std::size_t playerCount() const noexcept;

  /** @brief Ship of a player, if it has one right now. */
  [[nodiscard]] std::optional<ampersand::core::ecs::Entity> shipOf(
      std::uint8_t playerId) const;

  /** @brief The World of the game, to inspect it (tests). */
  [[nodiscard]] ampersand::core::ecs::World& world() noexcept;

 private:
  struct Player {
    std::optional<ampersand::core::ecs::Entity> ship;
    std::uint8_t inputMask{};
    std::uint8_t lives{STARTING_LIVES};
    std::uint32_t score{};
    /** @brief Seconds before the ship comes back, while it has none. */
    float respawnIn{};
    /** @brief Last state sent in a PlayerChanged. */
    std::optional<PlayerChanged> reported;
  };

  void updatePhase(float deltaTime, std::vector<GameEvent>& events);
  void start(std::vector<GameEvent>& events);
  void end(bool victory, std::vector<GameEvent>& events);
  /** @brief Kills every entity and forgets the ships. */
  void clear();
  void updatePlayers(float deltaTime);
  /** @brief Turns the keys held into the speed and trigger of a ship. */
  void steer(ampersand::core::ecs::Entity ship, std::uint8_t inputMask);
  void spawnShip(Player& player, std::uint8_t playerId);
  void keepShipsOnScreen();
  /** @brief Takes away lives and gives score from the Deaths of the tick. */
  void applyDeaths();
  void checkEnd(std::vector<GameEvent>& events);
  /** @brief Announces the new entities with a Kind, and the Deaths of the
   *         tick. */
  void announce(std::vector<GameEvent>& events);
  void reportPlayers(std::vector<GameEvent>& events);

  /** @brief Adds an entity to the Deaths, once. */
  void kill(ampersand::core::ecs::Entity entity, common::DeathReason reason);
  [[nodiscard]] bool isDying(ampersand::core::ecs::Entity entity);
  [[nodiscard]] bool hasPlayers() const noexcept;
  [[nodiscard]] PlayerChanged stateOf(std::uint8_t playerId,
                                      const Player& player);
  [[nodiscard]] EntityInfo infoOf(ampersand::core::ecs::Entity entity);

  ampersand::core::ecs::World _world;
  ampersand::scene::SceneManager _scenes;
  common::ShootingSystem _shootingSystem{_scenes};
  ampersand::physics::MovementSystem _movementSystem;
  ampersand::physics::CollisionSystem _collisionSystem;
  common::DamageSystem _damageSystem;
  common::OutOfScreenSystem _outOfScreenSystem{
      {0.0F, 0.0F, SCREEN_WIDTH, SCREEN_HEIGHT}};
  common::CleanupSystem _cleanupSystem;
  ampersand::scripting::ScriptingSystem _scriptingSystem;
  /** @brief Whether the scripts sent the last wave (finishWaves()). */
  bool _wavesFinished{};
  /** @brief Indexed by player id. */
  std::array<std::optional<Player>, protocol::MAX_PLAYERS> _players;
  Phase _phase{Phase::Waiting};
  float _restartIn{};
  std::uint32_t _nextEntityId{1};
};

}  // namespace rtype::server
