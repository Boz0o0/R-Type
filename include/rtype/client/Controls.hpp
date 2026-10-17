#pragma once

#include <ampersand/input/Event.hpp>
#include <array>
#include <cstdint>
#include <rtype/common/protocol/Protocol.hpp>
#include <set>

namespace rtype::client {

class Controls {
 public:
  [[nodiscard]] static std::uint8_t inputMask(
      const std::set<ampersand::input::Key>& keysDown);

 private:
  struct Binding {
    ampersand::input::Key key;
    std::uint8_t flag;
  };

  static constexpr std::array<Binding, 5> BINDINGS{{
      {.key = ampersand::input::Key::Up, .flag = protocol::InputFlag::MOVE_UP},
      {.key = ampersand::input::Key::Down,
       .flag = protocol::InputFlag::MOVE_DOWN},
      {.key = ampersand::input::Key::Left,
       .flag = protocol::InputFlag::MOVE_LEFT},
      {.key = ampersand::input::Key::Right,
       .flag = protocol::InputFlag::MOVE_RIGHT},
      {.key = ampersand::input::Key::Space, .flag = protocol::InputFlag::SHOOT},
  }};
};

}  // namespace rtype::client
