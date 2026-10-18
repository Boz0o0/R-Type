#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/core/resources/Clock.hpp>
#include <cstdint>
#include <random>
#include <rtype/common/Factories.hpp>
#include <rtype/server/systems/WaveSystem.hpp>

namespace rtype::server {

WaveSystem::WaveSystem(ampersand::core::math::Vec2f screen, std::uint32_t seed)
    : _screen(screen), _random(seed) {}

void WaveSystem::reset() {
  this->_nextWaveIn = FIRST_WAVE_DELAY;
  this->_wave = 0;
}

void WaveSystem::update(ampersand::core::ecs::World& world) {
  if (this->finished()) {
    return;
  }
  this->_nextWaveIn -= world.getResource<ampersand::core::Clock>().dt;
  if (this->_nextWaveIn <= 0.0F) {
    this->spawnWave(world);
    this->_nextWaveIn += WAVE_INTERVAL;
  }
}

bool WaveSystem::finished() const noexcept { return this->_wave >= WAVE_COUNT; }

std::uint32_t WaveSystem::wave() const noexcept { return this->_wave; }

void WaveSystem::spawnWave(ampersand::core::ecs::World& world) {
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
