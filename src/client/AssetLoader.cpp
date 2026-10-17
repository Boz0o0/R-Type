#include <ampersand/audio/IAudioEngine.hpp>
#include <ampersand/audio/SoundId.hpp>
#include <ampersand/render/FontId.hpp>
#include <ampersand/render/IRenderer.hpp>
#include <ampersand/render/TextureId.hpp>
#include <ampersand/render/components/Animation.hpp>
#include <ampersand/render/components/Sprite.hpp>
#include <cstddef>
#include <filesystem>
#include <rtype/client/AssetLoader.hpp>
#include <rtype/client/EntityLooks.hpp>
#include <span>
#include <stdexcept>

namespace rtype::client {

namespace audio = ampersand::audio;
namespace render = ampersand::render;

Assets AssetLoader::load(render::IRenderer& renderer,
                         audio::IAudioEngine& audio,
                         const std::filesystem::path& directory) {
  const std::filesystem::path sprites = directory / "sprites";
  const render::TextureId ships = texture(renderer, sprites / "ships.png");
  const render::TextureId enemy = texture(renderer, sprites / "enemy.png");
  const render::TextureId playerShots =
      texture(renderer, sprites / "player-shots.png");
  const render::TextureId enemyShots =
      texture(renderer, sprites / "enemy-shots.png");
  const render::TextureId explosions =
      texture(renderer, sprites / "explosions.png");

  Assets assets;
  for (std::size_t player = 0; player < assets.looks.ships.size(); ++player) {
    Frame frame = SHIP;
    frame.y += static_cast<std::int32_t>(player) * SHIP_ROW_HEIGHT;
    assets.looks.ships[player] = Look{
        .sprite = render::Sprite{.texture = ships, .source = frame},
        .scale = SHIP_SCALE,
    };
  }
  assets.looks.enemy =
      animated(enemy, ENEMY_FRAMES, ENEMY_FRAME_TIME, ENEMY_SCALE);
  assets.looks.playerMissile =
      animated(playerShots, PLAYER_SHOT_FRAMES, SHOT_FRAME_TIME, SHOT_SCALE);
  assets.looks.enemyMissile =
      animated(enemyShots, ENEMY_SHOT_FRAMES, SHOT_FRAME_TIME, SHOT_SCALE);
  assets.looks.explosion = animated(explosions, EXPLOSION_FRAMES,
                                    EXPLOSION_FRAME_TIME, EXPLOSION_SCALE);
  assets.looks.explosion.animation.loop = false;

  assets.looks.shootSound = sound(audio, directory / "sounds" / "shoot.wav");
  assets.looks.explosionSound =
      sound(audio, directory / "sounds" / "explosion.wav");
  assets.font = renderer.loadFont(directory / "fonts" / "tuffy.ttf");
  if (assets.font == 0) {
    throw std::runtime_error("cannot load the font from " + directory.string());
  }
  return assets;
}

render::TextureId AssetLoader::texture(render::IRenderer& renderer,
                                       const std::filesystem::path& path) {
  const render::TextureId texture = renderer.loadTexture(path);
  if (texture == 0) {
    throw std::runtime_error("cannot load " + path.string());
  }
  return texture;
}

audio::SoundId AssetLoader::sound(audio::IAudioEngine& audio,
                                  const std::filesystem::path& path) {
  const audio::SoundId sound = audio.loadSound(path);
  if (sound == 0) {
    throw std::runtime_error("cannot load " + path.string());
  }
  return sound;
}

Look AssetLoader::animated(render::TextureId texture,
                           std::span<const Frame> frames, float frameDuration,
                           float scale) {
  return Look{
      .sprite = render::Sprite{.texture = texture, .source = frames.front()},
      .animation =
          render::Animation{
              .frames = {frames.begin(), frames.end()},
              .frameDuration = frameDuration,
          },
      .scale = scale,
  };
}

}  // namespace rtype::client
