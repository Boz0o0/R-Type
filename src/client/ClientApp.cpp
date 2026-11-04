#include <ampersand/audio/systems/AudioSystem.hpp>
#include <ampersand/core/ecs/EventQueue.hpp>
#include <ampersand/core/ecs/Scheduler.hpp>
#include <ampersand/core/time/FrameTimer.hpp>
#include <ampersand/input/systems/InputSystem.hpp>
#include <ampersand/physics/systems/MovementSystem.hpp>
#include <ampersand/render/FontId.hpp>
#include <ampersand/render/resources/Renderer.hpp>
#include <ampersand/render/systems/AnimationSystem.hpp>
#include <ampersand/render/systems/RenderSystem.hpp>
#include <ampersand/scene/resources/SceneManager.hpp>
#include <ampersand/scripting/systems/ScriptingSystem.hpp>
#include <filesystem>
#include <optional>
#include <rtype/client/ClientApp.hpp>
#include <rtype/client/Hud.hpp>
#include <rtype/client/LobbyMenu.hpp>
#include <rtype/client/LoginMenu.hpp>
#include <rtype/client/PrefabFactory.hpp>
#include <rtype/client/WorldMirror.hpp>
#include <rtype/client/systems/CorrectionSystem.hpp>
#include <rtype/client/systems/ExplosionSystem.hpp>
#include <rtype/client/systems/LobbySystem.hpp>
#include <rtype/client/systems/LoginSystem.hpp>
#include <rtype/client/systems/NetworkSystem.hpp>
#include <rtype/client/systems/PredictionSystem.hpp>
#include <rtype/client/systems/QuitSystem.hpp>
#include <rtype/client/systems/ScriptErrorSystem.hpp>
#include <rtype/common/protocol/PacketCodec.hpp>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

namespace rtype::client {

namespace audio = ampersand::audio;
namespace core = ampersand::core;
namespace input = ampersand::input;
namespace physics = ampersand::physics;
namespace render = ampersand::render;
namespace scene = ampersand::scene;
namespace scripting = ampersand::scripting;

ClientApp::ClientApp(ClientConfig config)
    : _config(std::move(config)),
      _connection(this->_config.host(), this->_config.port()) {}

void ClientApp::run() {
  std::filesystem::current_path(this->_config.binaryDirectory());
  scene::SceneManager scenes;
  scenes.loadConfig(this->_world, std::string(CONFIG));
  auto& window = this->_world.getResource<render::Renderer>();
  if (!window.isOpen) {
    throw std::runtime_error("cannot open the window");
  }
  PrefabFactory factory(this->_world, scenes);
  WorldMirror worldMirror(this->_world, factory);
  const render::FontId font = scenes.asset(
      "font", std::string(FONT), [&window](const std::string& path) {
        return window.renderer.loadFont(path);
      });
  Hud hud(this->_world, font);
  LoginMenu loginMenu(this->_world, font, this->_config.name());
  LobbyMenu menu(this->_world, font);

  core::ecs::Scheduler scheduler(this->_world);
  scheduler.addEventQueue<scripting::ScriptError>();
  scheduler.addEventQueue<protocol::Payload>();
  scheduler.add<input::InputSystem>(pollWindow);
  scheduler.add<QuitSystem>(this->_connection);
  scheduler.add<NetworkSystem>(this->_connection, worldMirror, hud);
  scheduler.add<LoginSystem>(this->_connection, loginMenu);
  scheduler.add<LobbySystem>(this->_connection, menu,
                             this->_config.partyCode());
  scheduler.add<PredictionSystem>();
  scheduler.add<physics::MovementSystem>();
  scheduler.add<CorrectionSystem>();
  auto& scripts = scheduler.add<scripting::ScriptingSystem>();
  this->_connection.bindScripting(scripts, this->_world);
  scripts.lua()["arguments"] = scripts.lua().create_table_with(
      "username", this->_config.name(), "partyCode", this->_config.partyCode());
  scheduler.add<ScriptErrorSystem>();
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
