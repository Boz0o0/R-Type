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

/** @brief SFML keys reported to the engine: any other key is Key::Unknown. */
inline constexpr auto kKeys =
    std::to_array<std::pair<sf::Keyboard::Key, input::Key>>({
        {sf::Keyboard::Key::A, input::Key::A},
        {sf::Keyboard::Key::B, input::Key::B},
        {sf::Keyboard::Key::C, input::Key::C},
        {sf::Keyboard::Key::D, input::Key::D},
        {sf::Keyboard::Key::E, input::Key::E},
        {sf::Keyboard::Key::F, input::Key::F},
        {sf::Keyboard::Key::G, input::Key::G},
        {sf::Keyboard::Key::H, input::Key::H},
        {sf::Keyboard::Key::I, input::Key::I},
        {sf::Keyboard::Key::J, input::Key::J},
        {sf::Keyboard::Key::K, input::Key::K},
        {sf::Keyboard::Key::L, input::Key::L},
        {sf::Keyboard::Key::M, input::Key::M},
        {sf::Keyboard::Key::N, input::Key::N},
        {sf::Keyboard::Key::O, input::Key::O},
        {sf::Keyboard::Key::P, input::Key::P},
        {sf::Keyboard::Key::Q, input::Key::Q},
        {sf::Keyboard::Key::R, input::Key::R},
        {sf::Keyboard::Key::S, input::Key::S},
        {sf::Keyboard::Key::T, input::Key::T},
        {sf::Keyboard::Key::U, input::Key::U},
        {sf::Keyboard::Key::V, input::Key::V},
        {sf::Keyboard::Key::W, input::Key::W},
        {sf::Keyboard::Key::X, input::Key::X},
        {sf::Keyboard::Key::Y, input::Key::Y},
        {sf::Keyboard::Key::Z, input::Key::Z},
        {sf::Keyboard::Key::Num0, input::Key::Num0},
        {sf::Keyboard::Key::Num1, input::Key::Num1},
        {sf::Keyboard::Key::Num2, input::Key::Num2},
        {sf::Keyboard::Key::Num3, input::Key::Num3},
        {sf::Keyboard::Key::Num4, input::Key::Num4},
        {sf::Keyboard::Key::Num5, input::Key::Num5},
        {sf::Keyboard::Key::Num6, input::Key::Num6},
        {sf::Keyboard::Key::Num7, input::Key::Num7},
        {sf::Keyboard::Key::Num8, input::Key::Num8},
        {sf::Keyboard::Key::Num9, input::Key::Num9},
        {sf::Keyboard::Key::Up, input::Key::Up},
        {sf::Keyboard::Key::Down, input::Key::Down},
        {sf::Keyboard::Key::Left, input::Key::Left},
        {sf::Keyboard::Key::Right, input::Key::Right},
        {sf::Keyboard::Key::Space, input::Key::Space},
        {sf::Keyboard::Key::Enter, input::Key::Enter},
        {sf::Keyboard::Key::Escape, input::Key::Escape},
        {sf::Keyboard::Key::Tab, input::Key::Tab},
        {sf::Keyboard::Key::Backspace, input::Key::Backspace},
        {sf::Keyboard::Key::LShift, input::Key::LShift},
        {sf::Keyboard::Key::RShift, input::Key::RShift},
        {sf::Keyboard::Key::LControl, input::Key::LControl},
        {sf::Keyboard::Key::RControl, input::Key::RControl},
        {sf::Keyboard::Key::LAlt, input::Key::LAlt},
        {sf::Keyboard::Key::RAlt, input::Key::RAlt},
    });

inline input::Key toKey(sf::Keyboard::Key key) {
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
    return input::KeyPressed{toKey(pressed.code)};
}

inline std::optional<Event> toEvent(const sf::Event::KeyReleased& released) {
    return input::KeyReleased{toKey(released.code)};
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

/** @brief Any other SFML event is not reported to the engine. */
template <typename SfmlEvent>
std::optional<Event> toEvent(const SfmlEvent& /*other*/) {
    return std::nullopt;
}

}  // namespace ampersand::render
