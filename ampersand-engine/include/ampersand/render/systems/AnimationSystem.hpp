#pragma once

#include <ampersand/core/ecs/ISystem.hpp>

namespace ampersand::render {

/**
 * @brief Advances every Animation with the dt of the Clock resource and
 * shows its current frame on the entity's Sprite.
 */
class AnimationSystem final : public core::ecs::ISystem {
public:
    /**
     * @brief Adds dt to each Animation's elapsed time, then writes the
     * current frame into the Sprite::source of the same entity.
     *
     * Entities without a Sprite, and animations without frames, are skipped.
     *
     * @param world World holding the entities and the Clock resource.
     * @throws ResourceNotFound if the World has no Clock.
     * @throws std::invalid_argument if a frameDuration is not finite and
     * positive.
     */
    void update(core::ecs::World& world) override;
};

}  // namespace ampersand::render
