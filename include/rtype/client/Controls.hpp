#pragma once

#include <ampersand/input/Event.hpp>
#include <ampersand/input/resources/InputManager.hpp>
#include <cstdint>
#include <string_view>

namespace rtype::client {

class Controls {
 public:
  static constexpr std::string_view HORIZONTAL = "Horizontal";
  static constexpr std::string_view VERTICAL = "Vertical";
  static constexpr std::string_view FIRE = "Fire";
  static constexpr std::string_view QUIT = "Quit";

  static void bind(ampersand::input::InputManager& manager);
  [[nodiscard]] static std::uint8_t inputMask(
      const ampersand::input::InputManager& manager);

 private:
  // SFML numbers an Xbox pad's A button 0 and its Start button 7.
  static constexpr ampersand::input::JoystickButton FIRE_BUTTON =
      ampersand::input::JoystickButton::Button0;
  static constexpr ampersand::input::JoystickButton START_BUTTON =
      ampersand::input::JoystickButton::Button7;
};

}  // namespace rtype::client
