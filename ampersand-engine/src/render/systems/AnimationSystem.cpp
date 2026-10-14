#include <algorithm>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/resources/Clock.hpp>
#include <ampersand/render/components/Animation.hpp>
#include <ampersand/render/components/Sprite.hpp>
#include <ampersand/render/systems/AnimationSystem.hpp>
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace ampersand::render {

void AnimationSystem::update(core::ecs::World& world) {
    const float deltaTime = world.getResource<core::Clock>().dt;
    for (const core::ecs::Entity entity : world.owners<Animation>()) {
        auto& animation = world.get<Animation>(entity);
        if (!world.has<Sprite>(entity) || animation.frames.empty()) {
            continue;
        }
        if (!std::isfinite(animation.frameDuration) ||
            animation.frameDuration <= 0.0F) {
            throw std::invalid_argument(
                "Animation frameDuration must be finite and positive");
        }
        const float totalDuration = animation.frameDuration *
                                    static_cast<float>(animation.frames.size());
        animation.elapsed += deltaTime;
        if (animation.loop) {
            animation.elapsed = std::fmod(animation.elapsed, totalDuration);
        } else {
            animation.elapsed = std::min(animation.elapsed, totalDuration);
        }
        const auto frame =
            std::min(static_cast<std::size_t>(animation.elapsed /
                                              animation.frameDuration),
                     animation.frames.size() - 1);
        world.get<Sprite>(entity).source = animation.frames[frame];
    }
}

}  // namespace ampersand::render
