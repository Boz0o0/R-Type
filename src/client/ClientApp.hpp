#pragma once

#include <ampersand/core/ecs/World.hpp>
#include <ampersand/input/systems/InputSystem.hpp>
#include <ampersand/render/SfmlRenderer.hpp>
#include <ampersand/render/systems/RenderSystem.hpp>

#include "client/ClientConfig.hpp"

namespace rtype::client {

class ClientApp {
 public:
  explicit ClientApp(ClientConfig config);
  void run();

 private:
  ClientConfig _config;
  ampersand::render::SfmlRenderer _backend;
  ampersand::core::ecs::World _world;
  ampersand::input::InputSystem _input;
  ampersand::render::RenderSystem _render;
};

}  // namespace rtype::client
