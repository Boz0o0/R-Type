#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <ampersand/input/Event.hpp>
#include <ampersand/render/Color.hpp>
#include <ampersand/render/Event.hpp>
#include <array>
#include <optional>
#include <utility>

namespace ampersand::render {

inline sf::Color toSfColor(Color color) {
    return {color.r, color.g, color.b, color.a};
}

/**
 * @brief SFML scancodes reported to the engine, by physical position on the
 * keyboard: any other key is Key::Unknown.
 */
inline constexpr auto kKeys =
    std::to_array<std::pair<sf::Keyboard::Scancode, input::Key>>({
        {sf::Keyboard::Scan::A, input::Key::A},
        {sf::Keyboard::Scan::B, input::Key::B},
        {sf::Keyboard::Scan::C, input::Key::C},
        {sf::Keyboard::Scan::D, input::Key::D},
        {sf::Keyboard::Scan::E, input::Key::E},
        {sf::Keyboard::Scan::F, input::Key::F},
        {sf::Keyboard::Scan::G, input::Key::G},
        {sf::Keyboard::Scan::H, input::Key::H},
        {sf::Keyboard::Scan::I, input::Key::I},
        {sf::Keyboard::Scan::J, input::Key::J},
        {sf::Keyboard::Scan::K, input::Key::K},
        {sf::Keyboard::Scan::L, input::Key::L},
        {sf::Keyboard::Scan::M, input::Key::M},
        {sf::Keyboard::Scan::N, input::Key::N},
        {sf::Keyboard::Scan::O, input::Key::O},
        {sf::Keyboard::Scan::P, input::Key::P},
        {sf::Keyboard::Scan::Q, input::Key::Q},
        {sf::Keyboard::Scan::R, input::Key::R},
        {sf::Keyboard::Scan::S, input::Key::S},
        {sf::Keyboard::Scan::T, input::Key::T},
        {sf::Keyboard::Scan::U, input::Key::U},
        {sf::Keyboard::Scan::V, input::Key::V},
        {sf::Keyboard::Scan::W, input::Key::W},
        {sf::Keyboard::Scan::X, input::Key::X},
        {sf::Keyboard::Scan::Y, input::Key::Y},
        {sf::Keyboard::Scan::Z, input::Key::Z},
        {sf::Keyboard::Scan::Num0, input::Key::Num0},
        {sf::Keyboard::Scan::Num1, input::Key::Num1},
        {sf::Keyboard::Scan::Num2, input::Key::Num2},
        {sf::Keyboard::Scan::Num3, input::Key::Num3},
        {sf::Keyboard::Scan::Num4, input::Key::Num4},
        {sf::Keyboard::Scan::Num5, input::Key::Num5},
        {sf::Keyboard::Scan::Num6, input::Key::Num6},
        {sf::Keyboard::Scan::Num7, input::Key::Num7},
        {sf::Keyboard::Scan::Num8, input::Key::Num8},
        {sf::Keyboard::Scan::Num9, input::Key::Num9},
        {sf::Keyboard::Scan::Up, input::Key::Up},
        {sf::Keyboard::Scan::Down, input::Key::Down},
        {sf::Keyboard::Scan::Left, input::Key::Left},
        {sf::Keyboard::Scan::Right, input::Key::Right},
        {sf::Keyboard::Scan::Space, input::Key::Space},
        {sf::Keyboard::Scan::Enter, input::Key::Enter},
        {sf::Keyboard::Scan::Escape, input::Key::Escape},
        {sf::Keyboard::Scan::Tab, input::Key::Tab},
        {sf::Keyboard::Scan::Backspace, input::Key::Backspace},
        {sf::Keyboard::Scan::LShift, input::Key::LShift},
        {sf::Keyboard::Scan::RShift, input::Key::RShift},
        {sf::Keyboard::Scan::LControl, input::Key::LControl},
        {sf::Keyboard::Scan::RControl, input::Key::RControl},
        {sf::Keyboard::Scan::LAlt, input::Key::LAlt},
        {sf::Keyboard::Scan::RAlt, input::Key::RAlt},
    });

inline input::Key toKey(sf::Keyboard::Scancode key) {
    for (const auto& [sfmlKey, engineKey] : kKeys) {
        if (sfmlKey == key) {
            return engineKey;
        }
    }
    return input::Key::Unknown;
}

inline input::Button toButton(sf::Mouse::Button button) {
    switch (button) {
        case sf::Mouse::Button::Left:
            return input::Button::Left;
        case sf::Mouse::Button::Right:
            return input::Button::Right;
        case sf::Mouse::Button::Middle:
            return input::Button::Middle;
        default:
            return input::Button::Unknown;
    }
}

inline std::optional<Event> toEvent(const sf::Event::Closed& /*closed*/) {
    return Closed{};
}

inline std::optional<Event> toEvent(const sf::Event::Resized& resized) {
    return Resized{{resized.size.x, resized.size.y}};
}

inline std::optional<Event> toEvent(const sf::Event::KeyPressed& pressed) {
    return input::KeyPressed{toKey(pressed.scancode)};
}

inline std::optional<Event> toEvent(const sf::Event::KeyReleased& released) {
    return input::KeyReleased{toKey(released.scancode)};
}

inline std::optional<Event> toEvent(
    const sf::Event::MouseButtonPressed& pressed) {
    return input::MousePressed{
        .button = toButton(pressed.button),
        .position = {pressed.position.x, pressed.position.y},
    };
}

inline std::optional<Event> toEvent(
    const sf::Event::MouseButtonReleased& released) {
    return input::MouseReleased{
        .button = toButton(released.button),
        .position = {released.position.x, released.position.y},
    };
}

inline std::optional<Event> toEvent(const sf::Event::MouseMoved& moved) {
    return input::MouseMoved{{moved.position.x, moved.position.y}};
}

inline std::optional<Event> toEvent(const sf::Event::FocusLost& /*lost*/) {
    return input::FocusLost{};
}

/** @brief Any other SFML event is not reported to the engine. */
template <typename SfmlEvent>
std::optional<Event> toEvent(const SfmlEvent& /*other*/) {
    return std::nullopt;
}

}  // namespace ampersand::render
