#pragma once

#include <ampersand/core/math/Vec2.hpp>

namespace rtype::client {

// Our own ship, moved from our input without waiting for the server. The last
// server state is kept to put the ship back in place once both have stopped.
struct Predicted {
  ampersand::core::math::Vec2f serverPosition;
  ampersand::core::math::Vec2f serverVelocity;
};

}  // namespace rtype::client
