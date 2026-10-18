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

  static constexpr std::array BINDINGS{
      Binding{
          .key = ampersand::input::Key::Up,
          .flag = protocol::InputFlag::MOVE_UP,
      },
      Binding{
          .key = ampersand::input::Key::Down,
          .flag = protocol::InputFlag::MOVE_DOWN,
      },
      Binding{
          .key = ampersand::input::Key::Left,
          .flag = protocol::InputFlag::MOVE_LEFT,
      },
      Binding{
          .key = ampersand::input::Key::Right,
          .flag = protocol::InputFlag::MOVE_RIGHT,
      },
      Binding{
          .key = ampersand::input::Key::Space,
          .flag = protocol::InputFlag::SHOOT,
      },
  };
};

}  // namespace rtype::client
