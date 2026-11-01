#include <gtest/gtest.h>

#include <ampersand/core/math/Vec2.hpp>
#include <cmath>
#include <cstdint>
#include <rtype/common/ShipMovement.hpp>
#include <rtype/common/protocol/Protocol.hpp>

using ampersand::core::math::Vec2f;
using rtype::common::ShipMovement;
namespace InputFlag = rtype::protocol::InputFlag;

static std::uint8_t both(std::uint8_t first, std::uint8_t second) {
  return static_cast<std::uint8_t>(static_cast<unsigned>(first) |
                                   static_cast<unsigned>(second));
}

TEST(ShipMovement, StandsStillWithNoArrow) {
  const Vec2f velocity = ShipMovement::velocity(InputFlag::SHOOT);

  EXPECT_FLOAT_EQ(velocity.x, 0.0F);
  EXPECT_FLOAT_EQ(velocity.y, 0.0F);
}

TEST(ShipMovement, MovesAtFullSpeedOnOneAxis) {
  EXPECT_FLOAT_EQ(ShipMovement::velocity(InputFlag::MOVE_RIGHT).x,
                  ShipMovement::SPEED);
  EXPECT_FLOAT_EQ(ShipMovement::velocity(InputFlag::MOVE_LEFT).x,
                  -ShipMovement::SPEED);
  EXPECT_FLOAT_EQ(ShipMovement::velocity(InputFlag::MOVE_UP).y,
                  -ShipMovement::SPEED);
  EXPECT_FLOAT_EQ(ShipMovement::velocity(InputFlag::MOVE_DOWN).y,
                  ShipMovement::SPEED);
}

TEST(ShipMovement, KeepsTheSameSpeedOnDiagonals) {
  const Vec2f velocity =
      ShipMovement::velocity(both(InputFlag::MOVE_RIGHT, InputFlag::MOVE_UP));

  EXPECT_GT(velocity.x, 0.0F);
  EXPECT_LT(velocity.y, 0.0F);
  EXPECT_NEAR(std::hypot(velocity.x, velocity.y), ShipMovement::SPEED, 0.1F);
}

TEST(ShipMovement, OppositeArrowsCancelOut) {
  EXPECT_FLOAT_EQ(
      ShipMovement::velocity(both(InputFlag::MOVE_LEFT, InputFlag::MOVE_RIGHT))
          .x,
      0.0F);
}

TEST(ShipMovement, KeepsTheWholeShipOnScreen) {
  const Vec2f topLeft = ShipMovement::keepOnScreen({-10.0F, -10.0F});
  const Vec2f bottomRight = ShipMovement::keepOnScreen({5000.0F, 5000.0F});

  EXPECT_FLOAT_EQ(topLeft.x, 0.0F);
  EXPECT_FLOAT_EQ(topLeft.y, 0.0F);
  EXPECT_FLOAT_EQ(bottomRight.x, ShipMovement::SCREEN.x - ShipMovement::SIZE.x);
  EXPECT_FLOAT_EQ(bottomRight.y, ShipMovement::SCREEN.y - ShipMovement::SIZE.y);
}

TEST(ShipMovement, LeavesAShipOnScreenWhereItIs) {
  const Vec2f position = ShipMovement::keepOnScreen({300.0F, 400.0F});

  EXPECT_FLOAT_EQ(position.x, 300.0F);
  EXPECT_FLOAT_EQ(position.y, 400.0F);
}
