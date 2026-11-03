#include <algorithm>
#include <ampersand/core/math/Vec2.hpp>
#include <cstdint>
#include <rtype/common/ShipMovement.hpp>
#include <rtype/common/protocol/Protocol.hpp>

namespace rtype::common {

using ampersand::core::math::Vec2f;
namespace InputFlag = protocol::InputFlag;

Vec2f ShipMovement::velocity(std::uint8_t inputMask) {
  const auto held = [inputMask](std::uint8_t flag) {
    return (inputMask & flag) != 0 ? 1.0F : 0.0F;
  };
  const Vec2f arrows{held(InputFlag::MOVE_RIGHT) - held(InputFlag::MOVE_LEFT),
                     held(InputFlag::MOVE_DOWN) - held(InputFlag::MOVE_UP)};
  float speed = SPEED;
  if (arrows.x != 0.0F && arrows.y != 0.0F) {
    speed *= DIAGONAL_FACTOR;
  }
  return {arrows.x * speed, arrows.y * speed};
}

Vec2f ShipMovement::keepOnScreen(Vec2f position) {
  return {std::clamp(position.x, 0.0F, SCREEN.x - SIZE.x),
          std::clamp(position.y, 0.0F, SCREEN.y - SIZE.y)};
}

}  // namespace rtype::common
