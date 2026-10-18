#pragma once

#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/math/Vec2.hpp>
#include <cstdint>
#include <random>

namespace rtype::server {

/**
 * @brief Sends the waves of Bydos of a level, one every WAVE_INTERVAL.
 *
 * Wave n (from 0) has FIRST_WAVE_SIZE + n enemies, lined up on the right of
 * the screen at random heights, flying left. The level ends after WAVE_COUNT
 * waves.
 */
class WaveSpawner {
 public:
  /** @brief Waves in a level. */
  static constexpr std::uint32_t WAVE_COUNT = 10;
  /** @brief Enemies in the first wave; each next wave has one more. */
  static constexpr std::uint32_t FIRST_WAVE_SIZE = 3;
  /** @brief Seconds before the first wave. */
  static constexpr float FIRST_WAVE_DELAY = 2.0F;
  /** @brief Seconds between two waves. */
  static constexpr float WAVE_INTERVAL = 5.0F;
  /** @brief Horizontal gap between two enemies of a wave. */
  static constexpr float ENEMY_SPACING = 96.0F;
  /** @brief Free space above and below the enemies. */
  static constexpr float VERTICAL_MARGIN = 64.0F;
  /** @brief Height of an enemy (see createEnemy). */
  static constexpr float ENEMY_HEIGHT = 64.0F;

  /**
   * @brief Creates a spawner at the start of a level.
   *
   * @param screen Size of the screen: enemies appear just past its right
   *        edge, between its top and bottom.
   * @param seed Seed of the random heights.
   */
  WaveSpawner(ampersand::core::math::Vec2f screen, std::uint32_t seed);

  /** @brief Goes back to the start of the level. */
  void reset();

  /**
   * @brief Spawns the next wave when it is time to.
   *
   * @param world World to create the enemies in.
   * @param deltaTime Seconds since the previous call.
   * @throws ResourceNotFound if the World has no Spawns.
   */
  void update(ampersand::core::ecs::World& world, float deltaTime);

  /** @brief Whether every wave of the level was sent. */
  [[nodiscard]] bool finished() const noexcept;

  /** @brief Number of waves sent so far. */
  [[nodiscard]] std::uint32_t wave() const noexcept;

 private:
  void spawnWave(ampersand::core::ecs::World& world);

  ampersand::core::math::Vec2f _screen;
  std::mt19937 _random;
  float _nextWaveIn{FIRST_WAVE_DELAY};
  std::uint32_t _wave{0};
};

}  // namespace rtype::server
