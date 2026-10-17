#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/math/Vec2.hpp>
#include <cstdint>
#include <random>
#include <rtype/common/Factories.hpp>
#include <rtype/server/WaveSpawner.hpp>

namespace rtype::server {

WaveSpawner::WaveSpawner(ampersand::core::math::Vec2f screen,
                         std::uint32_t seed)
    : _screen(screen), _random(seed) {}

void WaveSpawner::reset() {
  this->_nextWaveIn = FIRST_WAVE_DELAY;
  this->_wave = 0;
}

void WaveSpawner::update(ampersand::core::ecs::World& world, float deltaTime) {
  if (this->finished()) {
    return;
  }
  this->_nextWaveIn -= deltaTime;
  if (this->_nextWaveIn <= 0.0F) {
    this->spawnWave(world);
    this->_nextWaveIn += WAVE_INTERVAL;
  }
}

bool WaveSpawner::finished() const noexcept {
  return this->_wave >= WAVE_COUNT;
}

std::uint32_t WaveSpawner::wave() const noexcept { return this->_wave; }

void WaveSpawner::spawnWave(ampersand::core::ecs::World& world) {
  std::uniform_real_distribution<float> height(
      VERTICAL_MARGIN, this->_screen.y - VERTICAL_MARGIN - ENEMY_HEIGHT);
  const std::uint32_t size = FIRST_WAVE_SIZE + this->_wave;
  for (std::uint32_t index = 0; index < size; ++index) {
    const float left =
        this->_screen.x + (static_cast<float>(index) * ENEMY_SPACING);
    common::createEnemy(world, {left, height(this->_random)});
  }
  ++this->_wave;
}

}  // namespace rtype::server
