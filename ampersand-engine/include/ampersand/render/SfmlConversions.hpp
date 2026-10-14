#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <ampersand/input/Event.hpp>
#include <ampersand/render/Color.hpp>

namespace ampersand::render {

inline sf::Color toSfColor(Color color) {
    return {color.r, color.g, color.b, color.a};
}

inline input::Key toKey(sf::Keyboard::Key key) {
    switch (key) {
        case sf::Keyboard::Key::Up:
            return input::Key::Up;
        case sf::Keyboard::Key::Down:
            return input::Key::Down;
        case sf::Keyboard::Key::Left:
            return input::Key::Left;
        case sf::Keyboard::Key::Right:
            return input::Key::Right;
        case sf::Keyboard::Key::Space:
            return input::Key::Space;
        case sf::Keyboard::Key::Escape:
            return input::Key::Escape;
        default:
            return input::Key::Unknown;
    }
}

}  // namespace ampersand::render
