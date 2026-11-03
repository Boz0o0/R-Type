#include <gtest/gtest.h>

#include <ampersand/input/Event.hpp>
#include <ampersand/input/resources/InputManager.hpp>
#include <rtype/client/systems/QuitSystem.hpp>

#include "GameFiles.hpp"

using ampersand::input::InputManager;
using ampersand::input::JoystickButton;
using ampersand::input::JoystickButtonPressed;
using ampersand::input::KeyCode;
using ampersand::input::KeyPressed;
using rtype::client::QuitSystem;
using rtype::client::test::gameInput;

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

TEST(QuitSystem, EscapeOrStartQuits) {
  EXPECT_TRUE(QuitSystem::shouldQuit(pressing(KeyCode::Escape), false));
  EXPECT_TRUE(QuitSystem::shouldQuit(pressing(JoystickButton::Button7), false));
}

TEST(QuitSystem, OtherInputsDoNotQuitDuringTheGame) {
  EXPECT_FALSE(QuitSystem::shouldQuit(gameInput(), false));
  EXPECT_FALSE(QuitSystem::shouldQuit(pressing(KeyCode::Space), false));
  EXPECT_FALSE(
      QuitSystem::shouldQuit(pressing(JoystickButton::Button0), false));
}

TEST(QuitSystem, AnyKeyOrButtonQuitsOnceTheConnectionEnded) {
  EXPECT_TRUE(QuitSystem::shouldQuit(pressing(KeyCode::Space), true));
  EXPECT_TRUE(QuitSystem::shouldQuit(pressing(JoystickButton::Button0), true));
}

TEST(QuitSystem, AKeyHeldFromBeforeDoesNotQuitTheEndScreen) {
  InputManager manager = pressing(KeyCode::Space);
  manager.beginFrame();

  EXPECT_FALSE(QuitSystem::shouldQuit(manager, true));
}
