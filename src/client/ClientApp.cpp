#include "client/ClientApp.hpp"

#include <ampersand/core/resources/Clock.hpp>
#include <ampersand/input/resources/InputManager.hpp>
#include <ampersand/render/resources/Camera.hpp>
#include <ampersand/render/resources/ClearColor.hpp>
#include <ampersand/render/resources/Renderer.hpp>
#include <chrono>
#include <optional>
#include <stdexcept>
#include <utility>
#include <variant>

namespace rtype::client {

namespace core = ampersand::core;
namespace input = ampersand::input;
namespace render = ampersand::render;

namespace {
constexpr float WORLD_WIDTH = 1920.0F;
constexpr float WORLD_HEIGHT = 1080.0F;

constexpr core::math::Vec2u WINDOW_SIZE{1280, 720};
constexpr render::Color SPACE_COLOR{.r = 4, .g = 4, .b = 14};

std::optional<input::Event> pollWindow(core::ecs::World& world) {
  auto& window = world.getResource<render::Renderer>();
  while (const auto event = window.renderer.pollEvent()) {
    if (std::holds_alternative<render::Closed>(*event)) {
      window.isOpen = false;
    } else if (const auto* pressed = std::get_if<input::KeyPressed>(&*event)) {
      return *pressed;
    } else if (const auto* released =
                   std::get_if<input::KeyReleased>(&*event)) {
      return *released;
    } else if (const auto* clicked =
                   std::get_if<input::MousePressed>(&*event)) {
      return *clicked;
    } else if (const auto* unclicked =
                   std::get_if<input::MouseReleased>(&*event)) {
      return *unclicked;
    } else if (const auto* moved = std::get_if<input::MouseMoved>(&*event)) {
      return *moved;
    } else if (std::holds_alternative<input::FocusLost>(*event)) {
      return input::FocusLost{};
    }
  }
  return std::nullopt;
}

}  // namespace

ClientApp::ClientApp(ClientConfig config)
    : _config(std::move(config)), _input(pollWindow) {}

void ClientApp::run() {
  auto& window = this->_world.addResource<render::Renderer>(
      this->_backend,
      render::WindowConfig{.size = WINDOW_SIZE, .title = "R-Type"});
  if (!window.isOpen) {
    throw std::runtime_error("cannot open the window");
  }
  this->_world.addResource<render::Camera>(
      render::Camera{.center = {WORLD_WIDTH / 2.0F, WORLD_HEIGHT / 2.0F},
                     .size = {WORLD_WIDTH, WORLD_HEIGHT}});
  this->_world.addResource<render::ClearColor>(
      render::ClearColor{.value = SPACE_COLOR});
  const auto& keys = this->_world.addResource<input::InputManager>();
  auto& clock = this->_world.addResource<core::Clock>();

  auto previous = std::chrono::steady_clock::now();
  while (window.isOpen) {
    const auto now = std::chrono::steady_clock::now();
    clock.dt = std::chrono::duration<float>(now - previous).count();
    clock.elapsed += clock.dt;
    previous = now;

    this->_input.update(this->_world);
    if (keys.keysDown.contains(input::Key::Escape)) {
      window.isOpen = false;
    }
    this->_render.update(this->_world);
  }
}

}  // namespace rtype::client
