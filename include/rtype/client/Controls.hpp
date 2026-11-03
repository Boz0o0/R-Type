#pragma once

#include <ampersand/input/resources/InputManager.hpp>
#include <cstdint>
#include <string_view>

namespace rtype::client {

// The input actions, bound to keys and buttons in game/world/config.json.
class Controls {
 public:
  static constexpr std::string_view HORIZONTAL = "Horizontal";
  static constexpr std::string_view VERTICAL = "Vertical";
  static constexpr std::string_view FIRE = "Fire";
  static constexpr std::string_view QUIT = "Quit";

  [[nodiscard]] static std::uint8_t inputMask(
      const ampersand::input::InputManager& manager);
};

}  // namespace rtype::client
