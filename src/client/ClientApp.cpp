#include <ampersand/audio/resources/AudioListener.hpp>
#include <ampersand/audio/systems/AudioSystem.hpp>
#include <ampersand/core/ecs/Scheduler.hpp>
#include <ampersand/core/time/FrameTimer.hpp>
#include <ampersand/input/systems/InputSystem.hpp>
#include <ampersand/physics/systems/MovementSystem.hpp>
#include <ampersand/render/resources/Camera.hpp>
#include <ampersand/render/resources/ClearColor.hpp>
#include <ampersand/render/resources/Renderer.hpp>
#include <ampersand/render/systems/AnimationSystem.hpp>
#include <ampersand/render/systems/RenderSystem.hpp>
#include <optional>
#include <rtype/client/AssetLoader.hpp>
#include <rtype/client/ClientApp.hpp>
#include <rtype/client/Controls.hpp>
#include <rtype/client/Hud.hpp>
#include <rtype/client/Starfield.hpp>
#include <rtype/client/WorldMirror.hpp>
#include <rtype/client/systems/ExplosionSystem.hpp>
#include <rtype/client/systems/NetworkSystem.hpp>
#include <rtype/client/systems/QuitSystem.hpp>
#include <rtype/client/systems/ScrollingSystem.hpp>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

namespace rtype::client {

namespace audio = ampersand::audio;
namespace core = ampersand::core;
namespace input = ampersand::input;
namespace physics = ampersand::physics;
namespace render = ampersand::render;

ClientApp::ClientApp(ClientConfig config)
    : _config(std::move(config)),
      _connection(this->_config.host(), this->_config.port(),
                  this->_config.name(), this->_config.partyCode()) {}

void ClientApp::run() {
  auto& window = this->_world.addResource<render::Renderer>(
      this->_backend,
      render::WindowConfig{.size = WINDOW_SIZE, .title = "R-Type"});
  if (!window.isOpen) {
    throw std::runtime_error("cannot open the window");
  }
  this->_world.addResource<render::Camera>(render::Camera{
      .center = {WORLD_WIDTH / 2.0F, WORLD_HEIGHT / 2.0F},
      .size = {WORLD_WIDTH, WORLD_HEIGHT},
  });
  this->_world.addResource<render::ClearColor>(
      render::ClearColor{.value = SPACE_COLOR});
  Controls::bind(this->_world.addResource<input::InputManager>());
  this->_world.addResource<audio::AudioListener>(
      audio::AudioListener{.engine = this->_audioEngine});
  const Assets assets = AssetLoader::load(this->_backend, this->_audioEngine,
                                          this->_config.assetsDirectory());
  Starfield::spawn(this->_world, assets.starfield);
  WorldMirror worldMirror(this->_world, assets.looks);
  Hud hud(this->_world, assets.font);

  core::ecs::Scheduler scheduler(this->_world);
  scheduler.add<input::InputSystem>(pollWindow);
  scheduler.add<QuitSystem>(this->_connection);
  scheduler.add<NetworkSystem>(this->_connection, worldMirror, hud);
  scheduler.add<physics::MovementSystem>();
  scheduler.add<ScrollingSystem>();
  scheduler.add<render::AnimationSystem>();
  scheduler.add<ExplosionSystem>();
  scheduler.add<audio::AudioSystem>();
  scheduler.add<render::RenderSystem>();

  core::FrameTimer timer;
  while (window.isOpen) {
    scheduler.update(timer.tick());
  }
  if (this->_connection.hasEnded()) {
    // The HUD showed why, and ServerConnection printed it.
    throw std::runtime_error("the connection to the server ended");
  }
  this->_connection.disconnect();
}

std::optional<input::Event> ClientApp::pollWindow(core::ecs::World& world) {
  auto& window = world.getResource<render::Renderer>();
  while (const auto event = window.renderer.pollEvent()) {
    if (std::holds_alternative<render::Closed>(*event)) {
      window.isOpen = false;
      continue;
    }
    // Every window event that the InputManager knows: keys, mouse, controllers.
    auto forwarded = std::visit(
        [](const auto& windowEvent) -> std::optional<input::Event> {
          if constexpr (std::is_constructible_v<input::Event,
                                                decltype(windowEvent)>) {
            return windowEvent;
          } else {
            return std::nullopt;
          }
        },
        *event);
    if (forwarded) {
      return forwarded;
    }
  }
  return std::nullopt;
}

}  // namespace rtype::client
