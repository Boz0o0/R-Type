#include <ampersand/input/Event.hpp>
#include <cstdint>
#include <rtype/client/Controls.hpp>
#include <set>

namespace rtype::client {

std::uint8_t Controls::inputMask(
    const std::set<ampersand::input::KeyCode>& keysDown) {
  std::uint8_t mask = 0;
  for (const Binding& binding : BINDINGS) {
    if (keysDown.contains(binding.key)) {
      mask |= binding.flag;
    }
  }
  return mask;
}

}  // namespace rtype::client
