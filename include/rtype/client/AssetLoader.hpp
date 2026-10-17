#pragma once

#include <ampersand/audio/IAudioEngine.hpp>
#include <ampersand/audio/SoundId.hpp>
#include <ampersand/core/math/Rect.hpp>
#include <ampersand/render/FontId.hpp>
#include <ampersand/render/IRenderer.hpp>
#include <ampersand/render/TextureId.hpp>
#include <array>
#include <cstdint>
#include <filesystem>
#include <rtype/client/EntityLooks.hpp>
#include <span>

namespace rtype::client {

struct Assets {
  EntityLooks looks;
  ampersand::render::FontId font{};
};

class AssetLoader {
 public:
  [[nodiscard]] static Assets load(ampersand::render::IRenderer& renderer,
                                   ampersand::audio::IAudioEngine& audio,
                                   const std::filesystem::path& directory);

 private:
  using Frame = ampersand::core::math::Rect<std::int32_t>;

  static constexpr Frame SHIP{67, 2, 32, 15};
  static constexpr std::int32_t SHIP_ROW_HEIGHT = 17;
  static constexpr std::array<Frame, 8> ENEMY_FRAMES{{
      {275, 5, 23, 26},
      {308, 5, 23, 26},
      {341, 5, 23, 26},
      {374, 5, 23, 26},
      {407, 5, 23, 26},
      {440, 5, 23, 26},
      {473, 5, 23, 26},
      {506, 5, 23, 26},
  }};
  static constexpr std::array<Frame, 2> PLAYER_SHOT_FRAMES{{
      {232, 103, 16, 12},
      {249, 103, 16, 12},
  }};
  static constexpr std::array<Frame, 8> ENEMY_SHOT_FRAMES{{
      {136, 6, 7, 6},
      {153, 6, 7, 6},
      {170, 6, 7, 6},
      {188, 6, 7, 6},
      {205, 6, 7, 6},
      {223, 6, 7, 6},
      {240, 6, 7, 6},
      {257, 6, 7, 6},
  }};
  static constexpr std::array<Frame, 6> EXPLOSION_FRAMES{{
      {129, 0, 33, 33},
      {162, 0, 33, 33},
      {195, 0, 33, 33},
      {228, 0, 33, 33},
      {261, 0, 33, 33},
      {294, 0, 33, 33},
  }};

  static constexpr float SHIP_SCALE = 2.0F;
  static constexpr float ENEMY_SCALE = 2.6F;
  static constexpr float SHOT_SCALE = 1.5F;
  static constexpr float EXPLOSION_SCALE = 2.5F;
  static constexpr float ENEMY_FRAME_TIME = 0.08F;
  static constexpr float SHOT_FRAME_TIME = 0.1F;
  static constexpr float EXPLOSION_FRAME_TIME = 0.07F;

  [[nodiscard]] static ampersand::render::TextureId texture(
      ampersand::render::IRenderer& renderer,
      const std::filesystem::path& path);
  [[nodiscard]] static ampersand::audio::SoundId sound(
      ampersand::audio::IAudioEngine& audio, const std::filesystem::path& path);
  [[nodiscard]] static Look animated(ampersand::render::TextureId texture,
                                     std::span<const Frame> frames,
                                     float frameDuration, float scale);
};

}  // namespace rtype::client
