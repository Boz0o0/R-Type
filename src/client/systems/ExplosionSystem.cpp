#include <ampersand/audio/components/AudioSource.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/render/components/Animation.hpp>
#include <rtype/client/components/Explosion.hpp>
#include <rtype/client/systems/ExplosionSystem.hpp>
#include <vector>

namespace rtype::client {

void ExplosionSystem::update(ampersand::core::ecs::World& world) {
  std::vector<ampersand::core::ecs::Entity> finished;
  for (const ampersand::core::ecs::Entity entity : world.owners<Explosion>()) {
    const bool animating = world.has<ampersand::render::Animation>(entity) &&
                           !ampersand::render::isFinished(
                               world.get<ampersand::render::Animation>(entity));
    const bool sounding =
        world.has<ampersand::audio::AudioSource>(entity) &&
        world.get<ampersand::audio::AudioSource>(entity).playing;
    if (!animating && !sounding) {
      finished.push_back(entity);
    }
  }
  // Destroying while looping would change the list we loop over.
  for (const ampersand::core::ecs::Entity entity : finished) {
    world.destroy(entity);
  }
}

}  // namespace rtype::client
