#pragma once

#include <ampersand/audio/IAudioEngine.hpp>
#include <ampersand/audio/MusicId.hpp>
#include <ampersand/audio/SoundId.hpp>
#include <ampersand/audio/VoiceId.hpp>
#include <ampersand/audio/resources/AudioListener.hpp>
#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/core/math/Vec3.hpp>
#include <ampersand/input/resources/InputManager.hpp>
#include <ampersand/render/Color.hpp>
#include <ampersand/render/Event.hpp>
#include <ampersand/render/FontId.hpp>
#include <ampersand/render/IRenderer.hpp>
#include <ampersand/render/Primitive.hpp>
#include <ampersand/render/TextureId.hpp>
#include <ampersand/render/WindowConfig.hpp>
#include <ampersand/render/components/Sprite.hpp>
#include <ampersand/render/components/Text.hpp>
#include <ampersand/render/resources/Camera.hpp>
#include <ampersand/render/resources/Renderer.hpp>
#include <ampersand/scene/resources/SceneManager.hpp>
#include <cstdint>
#include <filesystem>
#include <optional>

// The real files of game/, loaded without a window or a sound card: the
// Renderer and AudioListener of the configuration get fake backends, which
// still refuse a missing file.
namespace rtype::client::test {

namespace audio = ampersand::audio;
namespace core = ampersand::core;
namespace render = ampersand::render;

inline std::uint32_t idIfExists(const std::filesystem::path& path,
                                std::uint32_t& lastId) {
  return std::filesystem::is_regular_file(path) ? ++lastId : 0;
}

class FakeRenderer final : public render::IRenderer {
 public:
  bool open(const render::WindowConfig& /*config*/) override { return true; }
  void close() override {}
  [[nodiscard]] bool isOpen() const override { return true; }
  [[nodiscard]] core::math::Vec2u size() const override { return {1, 1}; }
  std::optional<render::Event> pollEvent() override { return std::nullopt; }
  void setView(const render::Camera& /*camera*/) override {}
  void resetView() override {}
  [[nodiscard]] render::TextureId loadTexture(
      const std::filesystem::path& path) override {
    return idIfExists(path, this->_lastId);
  }
  [[nodiscard]] render::FontId loadFont(
      const std::filesystem::path& path) override {
    return idIfExists(path, this->_lastId);
  }
  void beginFrame(render::Color /*clear*/) override {}
  void draw(const render::Sprite& /*sprite*/,
            const core::Transform& /*transform*/) override {}
  void draw(const render::Rectangle& /*rectangle*/,
            const core::Transform& /*transform*/) override {}
  void draw(const render::Circle& /*circle*/,
            const core::Transform& /*transform*/) override {}
  void draw(const render::Line& /*line*/,
            const core::Transform& /*transform*/) override {}
  void draw(const render::Text& /*text*/,
            const core::Transform& /*transform*/) override {}
  void endFrame() override {}

 private:
  std::uint32_t _lastId{0};
};

class FakeAudioEngine final : public audio::IAudioEngine {
 public:
  [[nodiscard]] audio::SoundId loadSound(
      const std::filesystem::path& path) override {
    return idIfExists(path, this->_lastId);
  }
  [[nodiscard]] audio::VoiceId play(audio::SoundId /*sound*/, float /*volume*/,
                                    float /*pitch*/, bool /*loop*/) override {
    return 0;
  }
  void stop(audio::VoiceId /*voice*/) override {}
  void pause(audio::VoiceId /*voice*/) override {}
  void resume(audio::VoiceId /*voice*/) override {}
  [[nodiscard]] bool isPlaying(audio::VoiceId /*voice*/) const override {
    return false;
  }
  void setVolume(audio::VoiceId /*voice*/, float /*volume*/) override {}
  void setPitch(audio::VoiceId /*voice*/, float /*pitch*/) override {}
  void setPosition(audio::VoiceId /*voice*/,
                   const core::math::Vec3f& /*position*/) override {}
  void setListenerPosition(const core::math::Vec3f& /*position*/) override {}
  void setGlobalVolume(float /*volume*/) override {}
  [[nodiscard]] audio::MusicId openMusic(
      const std::filesystem::path& path) override {
    return idIfExists(path, this->_lastId);
  }
  void playMusic(audio::MusicId /*music*/, float /*volume*/,
                 bool /*loop*/) override {}
  void setMusicVolume(audio::MusicId /*music*/, float /*volume*/) override {}
  void pauseMusic(audio::MusicId /*music*/) override {}
  void stopMusic(audio::MusicId /*music*/) override {}
  [[nodiscard]] bool isMusicPlaying(audio::MusicId /*music*/) const override {
    return false;
  }

 private:
  std::uint32_t _lastId{0};
};

// Must outlive the World and the SceneManager that use them.
struct FakeBackends {
  FakeRenderer renderer;
  FakeAudioEngine audio;
};

// Runs from the repository, like the client runs from its binary folder.
inline void loadGame(ampersand::scene::SceneManager& scenes,
                     core::ecs::World& world, FakeBackends& backends) {
  std::filesystem::current_path(RTYPE_SOURCE_DIR);
  scenes.registerResource<render::Renderer>(
      "Renderer",
      [&backends](core::ecs::World& target,
                  const ampersand::scene::json& data) {
        target.addResource<render::Renderer>(backends.renderer,
                                             data.get<render::WindowConfig>());
      },
      ampersand::scene::ResourceScope::Config);
  scenes.registerResource<audio::AudioListener>(
      "AudioListener",
      [&backends](core::ecs::World& target,
                  const ampersand::scene::json& /*data*/) {
        target.addResource<audio::AudioListener>(
            audio::AudioListener{.engine = backends.audio});
      },
      ampersand::scene::ResourceScope::Config);
  scenes.loadConfig(world, "game/world/config.json");
}

// The input actions of game/world/config.json, ready for a new frame.
inline ampersand::input::InputManager gameInput() {
  FakeBackends backends;
  ampersand::scene::SceneManager scenes;
  core::ecs::World world;
  loadGame(scenes, world, backends);
  ampersand::input::InputManager manager =
      world.getResource<ampersand::input::InputManager>();
  manager.beginFrame();
  return manager;
}

}  // namespace rtype::client::test
