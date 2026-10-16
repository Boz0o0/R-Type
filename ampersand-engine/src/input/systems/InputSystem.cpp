#include <ampersand/core/ecs/World.hpp>
#include <ampersand/input/resources/InputManager.hpp>
#include <ampersand/input/systems/InputSystem.hpp>
#include <optional>
#include <utility>
#include <variant>

namespace ampersand::input {

InputSystem::InputSystem(Poll poll) : _poll(std::move(poll)) {}

void InputSystem::update(core::ecs::World& world) {
    auto& manager = world.getResource<InputManager>();
    manager.events.clear();
    while (const std::optional<Event> event = _poll(world)) {
        std::visit([&manager](const auto& value) { apply(manager, value); },
                   *event);
    }
}

void InputSystem::apply(InputManager& manager, const KeyPressed& pressed) {
    if (manager.keysDown.insert(pressed.key).second) {
        manager.events.emplace_back(pressed);
    }
}

void InputSystem::apply(InputManager& manager, const KeyReleased& released) {
    manager.keysDown.erase(released.key);
    manager.events.emplace_back(released);
}

void InputSystem::apply(InputManager& manager, const MousePressed& pressed) {
    manager.mouse = pressed.position;
    if (manager.buttonsDown.insert(pressed.button).second) {
        manager.events.emplace_back(pressed);
    }
}

void InputSystem::apply(InputManager& manager, const MouseReleased& released) {
    manager.mouse = released.position;
    manager.buttonsDown.erase(released.button);
    manager.events.emplace_back(released);
}

void InputSystem::apply(InputManager& manager, const MouseMoved& moved) {
    manager.mouse = moved.position;
    manager.events.emplace_back(moved);
}

void InputSystem::apply(InputManager& manager, const FocusLost& /*lost*/) {
    for (const Key key : manager.keysDown) {
        manager.events.emplace_back(KeyReleased{key});
    }
    for (const Button button : manager.buttonsDown) {
        manager.events.emplace_back(
            MouseReleased{.button{button}, .position{manager.mouse}});
    }
    manager.keysDown.clear();
    manager.buttonsDown.clear();
}

}  // namespace ampersand::input
