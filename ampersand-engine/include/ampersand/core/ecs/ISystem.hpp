#pragma once

namespace ampersand::core::ecs {

class World;

/**
 * @brief Common interface of every system (game logic run on the World each
 * frame).
 */
class ISystem {
public:
    /** @brief Destroys the system. */
    virtual ~ISystem() = default;

    /**
     * @brief Runs the system once on the World.
     *
     * @param world World holding the entities, components and resources.
     */
    virtual void update(World& world) = 0;

    ISystem(const ISystem&) = delete;
    ISystem& operator=(const ISystem&) = delete;

protected:
    /** @brief Creates the base of a system. */
    ISystem() = default;
};

}  // namespace ampersand::core::ecs
