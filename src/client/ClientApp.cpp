#include <algorithm>
#include <ampersand/audio/resources/AudioListener.hpp>
#include <ampersand/core/resources/Clock.hpp>
#include <ampersand/render/resources/Camera.hpp>
#include <ampersand/render/resources/ClearColor.hpp>
#include <ampersand/render/resources/Renderer.hpp>
#include <chrono>
#include <optional>
#include <rtype/client/AssetLoader.hpp>
#include <rtype/client/ClientApp.hpp>
#include <rtype/client/Controls.hpp>
#include <rtype/client/Hud.hpp>
#include <rtype/client/WorldMirror.hpp>
#include <rtype/common/protocol/PacketCodec.hpp>
#include <rtype/common/protocol/Packets.hpp>
#include <stdexcept>
#include <utility>
#include <variant>

namespace rtype::client {

namespace audio = ampersand::audio;
namespace core = ampersand::core;
namespace input = ampersand::input;
namespace render = ampersand::render;

ClientApp::ClientApp(ClientConfig config)
    : _config(std::move(config)),
      _connection(this->_config.host(), this->_config.port(),
                  this->_config.name()),
      _inputSystem(pollWindow) {}

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
  const auto& keys = this->_world.addResource<input::InputManager>();
  auto& clock = this->_world.addResource<core::Clock>();
  this->_world.addResource<audio::AudioListener>(
      audio::AudioListener{.engine = this->_audioEngine});
  const Assets assets = AssetLoader::load(this->_backend, this->_audioEngine,
                                          this->_config.assetsDirectory());
  WorldMirror worldMirror(this->_world, assets.looks);
  Hud hud(this->_world, assets.font);

  auto previous = std::chrono::steady_clock::now();
  bool ended = false;
  while (window.isOpen) {
    const auto now = std::chrono::steady_clock::now();
    clock.dt = std::min(std::chrono::duration<float>(now - previous).count(),
                        MAX_FRAME_TIME);
    clock.elapsed += clock.dt;
    previous = now;

    this->_inputSystem.update(this->_world);
    if (keys.keysDown.contains(input::Key::Escape) ||
        (ended && pressedAKey(keys))) {
      window.isOpen = false;
    }
    this->_connection.update(now);
    hud.showConnection(this->_connection.state(), this->_connection.playerId(),
                       this->_connection.rejectReason());
    ended = this->_connection.hasEnded();
    for (const protocol::Payload& packet : this->_connection.takePackets()) {
      applyPacket(worldMirror, hud, packet);
    }
    this->_connection.sendInput(Controls::inputMask(keys.keysDown));
    this->_movementSystem.update(this->_world);
    this->_animationSystem.update(this->_world);
    this->_explosionSystem.update(this->_world);
    this->_audioSystem.update(this->_world);
    this->_renderSystem.update(this->_world);
  }
  if (this->_connection.hasEnded()) {
    // The HUD showed why, and ServerConnection printed it.
    throw std::runtime_error("the connection to the server ended");
  }
  this->_connection.disconnect();
}

bool ClientApp::pressedAKey(const input::InputManager& keys) {
  return std::ranges::any_of(keys.events, [](const input::Event& event) {
    return std::holds_alternative<input::KeyPressed>(event);
  });
}

void ClientApp::applyPacket(WorldMirror& worldMirror, Hud& hud,
                            const protocol::Payload& packet) {
  hud.apply(packet);
  if (const auto* spawn = std::get_if<protocol::EntitySpawn>(&packet)) {
    worldMirror.spawn(*spawn);
  } else if (const auto* snapshot =
                 std::get_if<protocol::WorldSnapshot>(&packet)) {
    worldMirror.applySnapshot(*snapshot);
  } else if (const auto* destroyed =
                 std::get_if<protocol::EntityDestroy>(&packet)) {
    worldMirror.destroy(*destroyed);
  }
}

std::optional<input::Event> ClientApp::pollWindow(core::ecs::World& world) {
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

}  // namespace rtype::client
