#pragma once

#include <ampersand/input/resources/InputManager.hpp>
#include <cstdint>
#include <string_view>

namespace rtype::client {

/** @brief The input actions of the player, bound to keys and buttons in
 *         game/world/config.json, and the input they send. */
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
   * @brief Turns the actions held into the input sent to the server. Two
   *        opposite directions cancel out.
   *
   * @param manager InputManager loaded with the actions of the config.
   * @return A mask of protocol::InputFlag.
   */
  [[nodiscard]] static std::uint8_t inputMask(
      const ampersand::input::InputManager& manager);
};

}  // namespace rtype::client
