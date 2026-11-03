#pragma once

#include <ampersand/input/Event.hpp>
#include <ampersand/input/resources/InputManager.hpp>
#include <cstdint>
#include <string_view>

namespace rtype::client {

/** @brief The input actions of the player, and the input they send. */
class Controls {
 public:
  /** @brief Axis: right arrow or stick right is 1, left is -1. */
  static constexpr std::string_view HORIZONTAL = "Horizontal";
  /** @brief Axis: up arrow or stick up is 1, down is -1. */
  static constexpr std::string_view VERTICAL = "Vertical";
  /** @brief Button: Space or the A button. */
  static constexpr std::string_view FIRE = "Fire";
  /** @brief Button: Escape or the Start button. */
  static constexpr std::string_view QUIT = "Quit";

  /**
   * @brief Declares the actions and their keys and controller buttons.
   *
   * @param manager InputManager to set up, once before the loop.
   */
  static void bind(ampersand::input::InputManager& manager);

  /**
   * @brief Turns the actions held into the input sent to the server. Two
   *        opposite directions cancel out.
   *
   * @param manager InputManager set up by bind().
   * @return A mask of protocol::InputFlag.
   */
  [[nodiscard]] static std::uint8_t inputMask(
      const ampersand::input::InputManager& manager);

 private:
  /** @brief SFML numbers an Xbox pad's A button 0 and its Start button 7. */
  static constexpr ampersand::input::JoystickButton FIRE_BUTTON =
      ampersand::input::JoystickButton::Button0;
  static constexpr ampersand::input::JoystickButton START_BUTTON =
      ampersand::input::JoystickButton::Button7;
};

}  // namespace rtype::client
