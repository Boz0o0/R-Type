#include <ampersand/input/resources/InputManager.hpp>
#include <cstdint>
#include <rtype/client/Controls.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <string>

namespace rtype::client {

namespace input = ampersand::input;

std::uint8_t Controls::inputMask(const input::InputManager& manager) {
  std::uint8_t mask = 0;
  const float vertical = manager.getAxisRaw(std::string(VERTICAL));
  if (vertical > 0.0F) {
    mask |= protocol::InputFlag::MOVE_UP;
  } else if (vertical < 0.0F) {
    mask |= protocol::InputFlag::MOVE_DOWN;
  }
  const float horizontal = manager.getAxisRaw(std::string(HORIZONTAL));
  if (horizontal > 0.0F) {
    mask |= protocol::InputFlag::MOVE_RIGHT;
  } else if (horizontal < 0.0F) {
    mask |= protocol::InputFlag::MOVE_LEFT;
  }
  if (manager.getButton(std::string(FIRE))) {
    mask |= protocol::InputFlag::SHOOT;
  }
  return mask;
}

}  // namespace rtype::client
