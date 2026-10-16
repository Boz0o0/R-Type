#pragma once

#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/input/Event.hpp>
#include <set>
#include <variant>
#include <vector>

namespace ampersand::input {

/** @brief Any input event. */
using Event = std::variant<KeyPressed, KeyReleased, MousePressed, MouseReleased,
                           MouseMoved>;

/**
 * @brief Input of the current frame: the events that happened, the keys and
 * mouse buttons that are held, and where the mouse is.
 *
 * Plain shared data, filled each frame by the InputSystem. The systems of the
 * game only read it.
 */
struct InputManager {
    /** @brief Events of the current frame, in order. */
    std::vector<Event> events;
    /** @brief Keys currently held. */
    std::set<Key> keysDown;
    /** @brief Mouse buttons currently held. */
    std::set<Button> buttonsDown;
    /** @brief Last known position of the mouse in the window, in pixels. */
    core::math::Vec2i mouse;
};

}  // namespace ampersand::input
