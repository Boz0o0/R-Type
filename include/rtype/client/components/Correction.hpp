#pragma once

#include <ampersand/core/math/Vec2.hpp>

namespace rtype::client {

// How far the entity is from where the server says it should be. It is
// absorbed a bit every frame, so a correction slides instead of teleporting.
struct Correction {
  ampersand::core::math::Vec2f offset;
};

}  // namespace rtype::client
