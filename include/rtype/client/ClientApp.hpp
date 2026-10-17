#pragma once

#include <ampersand/audio/SfmlAudioEngine.hpp>
#include <ampersand/audio/systems/AudioSystem.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/input/resources/InputManager.hpp>
#include <ampersand/input/systems/InputSystem.hpp>
#include <ampersand/physics/systems/MovementSystem.hpp>
#include <ampersand/render/Color.hpp>
#include <ampersand/render/SfmlRenderer.hpp>
#include <ampersand/render/systems/AnimationSystem.hpp>
#include <ampersand/render/systems/RenderSystem.hpp>
#include <chrono>
#include <cstdint>
#include <optional>
#include <rtype/client/ClientConfig.hpp>
#include <rtype/client/Hud.hpp>
#include <rtype/client/ServerConnection.hpp>
#include <rtype/client/WorldMirror.hpp>
#include <rtype/client/systems/ExplosionSystem.hpp>
#include <rtype/common/protocol/PacketCodec.hpp>

namespace rtype::client {

class ClientApp {
 public:
  static constexpr int32_t EXIT_ERROR = 84;

  explicit ClientApp(ClientConfig config);
  void run();

 private:
  static constexpr float WORLD_WIDTH = 1920.0F;
  static constexpr float WORLD_HEIGHT = 1080.0F;
  static constexpr ampersand::core::math::Vec2u WINDOW_SIZE{1280, 720};
  static constexpr ampersand::render::Color SPACE_COLOR{
      .r = 4,
      .g = 4,
      .b = 14,
  };

  static constexpr float MAX_FRAME_TIME = 0.1F;
  static constexpr std::chrono::seconds END_SCREEN_TIME{3};

  static std::optional<ampersand::input::Event> pollWindow(
      ampersand::core::ecs::World& world);
  static void applyPacket(WorldMirror& worldMirror, Hud& hud,
                          const protocol::Payload& packet);

  ClientConfig _config;
  ServerConnection _connection;
  ampersand::render::SfmlRenderer _backend;
  ampersand::audio::SfmlAudioEngine _audioEngine;
  ampersand::core::ecs::World _world;
  ampersand::input::InputSystem _inputSystem;
  ampersand::physics::MovementSystem _movementSystem;
  ampersand::render::AnimationSystem _animationSystem;
  ExplosionSystem _explosionSystem;
  ampersand::audio::AudioSystem _audioSystem;
  ampersand::render::RenderSystem _renderSystem;
};

}  // namespace rtype::client
