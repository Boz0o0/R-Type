#pragma once

#include <ampersand/input/Event.hpp>
#include <array>
#include <cstdint>
#include <rtype/common/protocol/Protocol.hpp>
#include <set>

namespace rtype::client {

/** @brief The keys of the game: the arrows move the ship, Space shoots. */
class Controls {
 public:
  /**
   * @brief Turns the keys held into the input sent to the server.
   *
   * @param keysDown The keys held this frame.
   * @return A mask of protocol::InputFlag.
   */
  [[nodiscard]] static std::uint8_t inputMask(
      const std::set<ampersand::input::KeyCode>& keysDown);

 private:
  /** @brief A key and the input it gives. */
  struct Binding {
    /** @brief The key. */
    ampersand::input::KeyCode key;
    /** @brief Its protocol::InputFlag. */
    std::uint8_t flag;
  };

  /** @brief Every key of the game. */
  static constexpr std::array BINDINGS{
      Binding{
          .key = ampersand::input::KeyCode::UpArrow,
          .flag = protocol::InputFlag::MOVE_UP,
      },
      Binding{
          .key = ampersand::input::KeyCode::DownArrow,
          .flag = protocol::InputFlag::MOVE_DOWN,
      },
      Binding{
          .key = ampersand::input::KeyCode::LeftArrow,
          .flag = protocol::InputFlag::MOVE_LEFT,
      },
      Binding{
          .key = ampersand::input::KeyCode::RightArrow,
          .flag = protocol::InputFlag::MOVE_RIGHT,
      },
      Binding{
          .key = ampersand::input::KeyCode::Space,
          .flag = protocol::InputFlag::SHOOT,
      },
  };
};

}  // namespace rtype::client
