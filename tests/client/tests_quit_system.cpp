#include <gtest/gtest.h>

#include <ampersand/input/Event.hpp>
#include <ampersand/input/resources/InputManager.hpp>
#include <rtype/client/ServerConnection.hpp>
#include <rtype/client/systems/QuitSystem.hpp>

#include "GameFiles.hpp"

using ampersand::input::InputManager;
using ampersand::input::JoystickButton;
using ampersand::input::JoystickButtonPressed;
using ampersand::input::KeyCode;
using ampersand::input::KeyPressed;
using rtype::client::QuitSystem;
using rtype::client::test::gameInput;
using Action = QuitSystem::Action;
using State = rtype::client::ServerConnection::State;

static InputManager pressing(KeyCode key) {
  InputManager manager = gameInput();
  manager.apply(KeyPressed{.key = key});
  return manager;
}

static InputManager pressing(JoystickButton button) {
  InputManager manager = gameInput();
  manager.apply(JoystickButtonPressed{.joystick = 0, .button = button});
  return manager;
}

TEST(QuitSystem, EscapeOrStartQuitsOutsideARoom) {
  EXPECT_EQ(
      QuitSystem::actionFor(pressing(KeyCode::Escape), State::InLobby, false),
      Action::Quit);
  EXPECT_EQ(QuitSystem::actionFor(pressing(JoystickButton::Button7),
                                  State::Connecting, false),
            Action::Quit);
}

TEST(QuitSystem, EscapeOrStartLeavesTheRoom) {
  EXPECT_EQ(
      QuitSystem::actionFor(pressing(KeyCode::Escape), State::InRoom, false),
      Action::LeaveRoom);
  EXPECT_EQ(QuitSystem::actionFor(pressing(JoystickButton::Button7),
                                  State::InRoom, false),
            Action::LeaveRoom);
}

// The press that left the room is still held on the next frame, in the lobby.
TEST(QuitSystem, EscapeHeldFromBeforeDoesNothing) {
  InputManager manager = pressing(KeyCode::Escape);
  manager.beginFrame();

  EXPECT_EQ(QuitSystem::actionFor(manager, State::Leaving, false),
            Action::None);
}

TEST(QuitSystem, OtherInputsDoNothingDuringTheGame) {
  EXPECT_EQ(QuitSystem::actionFor(gameInput(), State::InRoom, false),
            Action::None);
  EXPECT_EQ(
      QuitSystem::actionFor(pressing(KeyCode::Space), State::InRoom, false),
      Action::None);
  EXPECT_EQ(QuitSystem::actionFor(pressing(JoystickButton::Button0),
                                  State::InRoom, false),
            Action::None);
}

TEST(QuitSystem, AnyKeyOrButtonQuitsOnceTheConnectionEnded) {
  EXPECT_EQ(QuitSystem::actionFor(pressing(KeyCode::Space), State::Disconnected,
                                  true),
            Action::Quit);
  EXPECT_EQ(QuitSystem::actionFor(pressing(JoystickButton::Button0),
                                  State::Rejected, true),
            Action::Quit);
}

TEST(QuitSystem, AKeyHeldFromBeforeDoesNotQuitTheEndScreen) {
  InputManager manager = pressing(KeyCode::Space);
  manager.beginFrame();

  EXPECT_EQ(QuitSystem::actionFor(manager, State::Disconnected, true),
            Action::None);
}
