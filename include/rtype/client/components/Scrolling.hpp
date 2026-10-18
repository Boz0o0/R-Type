#pragma once

namespace rtype::client {

// One of two copies placed side by side: once it is fully off the left of the
// screen, it jumps behind the other copy.
struct Scrolling {
  float width{};
};

}  // namespace rtype::client
