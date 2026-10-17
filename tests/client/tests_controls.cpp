#include <gtest/gtest.h>

#include <ampersand/input/Event.hpp>
#include <rtype/client/Controls.hpp>
#include <rtype/common/protocol/Protocol.hpp>

using ampersand::input::Key;
using rtype::client::Controls;
namespace InputFlag = rtype::protocol::InputFlag;

TEST(Controls, NoKeyGivesAnEmptyMask) { EXPECT_EQ(Controls::inputMask({}), 0); }

TEST(Controls, EachArrowMovesTheShip) {
  EXPECT_EQ(Controls::inputMask({Key::Up}), InputFlag::MOVE_UP);
  EXPECT_EQ(Controls::inputMask({Key::Down}), InputFlag::MOVE_DOWN);
  EXPECT_EQ(Controls::inputMask({Key::Left}), InputFlag::MOVE_LEFT);
  EXPECT_EQ(Controls::inputMask({Key::Right}), InputFlag::MOVE_RIGHT);
}

TEST(Controls, SpaceShoots) {
  EXPECT_EQ(Controls::inputMask({Key::Space}), InputFlag::SHOOT);
}

TEST(Controls, CombinesKeysHeldTogether) {
  EXPECT_EQ(Controls::inputMask({Key::Up, Key::Right, Key::Space}),
            InputFlag::MOVE_UP | InputFlag::MOVE_RIGHT | InputFlag::SHOOT);
}

TEST(Controls, AllBoundKeysGiveEveryFlag) {
  EXPECT_EQ(Controls::inputMask(
                {Key::Up, Key::Down, Key::Left, Key::Right, Key::Space}),
            InputFlag::ALL);
}

TEST(Controls, IgnoresOtherKeys) {
  EXPECT_EQ(Controls::inputMask({Key::Escape, Key::A}), 0);
}
