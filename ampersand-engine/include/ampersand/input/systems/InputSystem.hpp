#pragma once

#include <ampersand/core/ecs/ISystem.hpp>
#include <ampersand/input/resources/InputManager.hpp>
#include <functional>
#include <optional>

namespace ampersand::input {

/**
 * @brief Fills the InputManager resource each frame with the events given by
 * a poll function.
 *
 * The input module does not know the window: the game gives the poll function
 * at construction (reading the Renderer resource of the World, a replay, a
 * test...) and handles the other events of the window (closing, resizing)
 * inside it.
 */
class InputSystem final : public core::ecs::ISystem {
public:
    /**
     * @brief Gives the next input event, or std::nullopt when none is left.
     * Receives the World given to update.
     */
    using Poll = std::function<std::optional<Event>(core::ecs::World&)>;

    /**
     * @brief Creates the system.
     *
     * @param poll Function giving the input events of the frame, one per call.
     * Must not be empty.
     */
    explicit InputSystem(Poll poll);

    /**
     * @brief Empties the events of the InputManager, then calls the poll
     * function with the World until it returns std::nullopt, adding each event
     * and keeping keysDown, buttonsDown and mouse up to date.
     *
     * A press of a key or button already held is ignored (the keyboard
     * repeats a held key).
     *
     * @param world World holding the InputManager resource.
     * @throws ResourceNotFound if the World has no InputManager.
     */
    void update(core::ecs::World& world) override;

private:
    /** @brief Holds the key, and adds the event if it was not held yet. */
    static void apply(InputManager& manager, const KeyPressed& pressed);

    /** @brief Releases the key and adds the event. */
    static void apply(InputManager& manager, const KeyReleased& released);

    /**
     * @brief Moves the mouse, holds the button, and adds the event if it was
     * not held yet.
     */
    static void apply(InputManager& manager, const MousePressed& pressed);

    /** @brief Moves the mouse, releases the button and adds the event. */
    static void apply(InputManager& manager, const MouseReleased& released);

    /** @brief Moves the mouse and adds the event. */
    static void apply(InputManager& manager, const MouseMoved& moved);

    /** @brief Releases each held key and button, adding their events. */
    static void apply(InputManager& manager, const FocusLost& lost);

    Poll _poll;
};

}  // namespace ampersand::input
