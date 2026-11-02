#pragma once

#include <ampersand/core/ecs/World.hpp>
#include <ampersand/render/TextureId.hpp>
#include <array>
#include <cstddef>
#include <cstdint>

namespace rtype::client {

class Starfield {
 public:
  static constexpr std::size_t LAYER_COUNT = 2;
  using Textures = std::array<ampersand::render::TextureId, LAYER_COUNT>;

  static void spawn(ampersand::core::ecs::World& world,
                    const Textures& textures);

 private:
  struct Layer {
    float speed{};
    std::int32_t depth{};
  };

  // The images are 960x540, drawn twice as big to cover the 1920x1080 world.
  static constexpr float WIDTH = 1920.0F;
  static constexpr float SCALE = 2.0F;
  static constexpr std::array<Layer, LAYER_COUNT> LAYERS{{
      {.speed = 30.0F, .depth = -20},
      {.speed = 90.0F, .depth = -10},
  }};

  static void addCopy(ampersand::core::ecs::World& world,
                      ampersand::render::TextureId texture, const Layer& layer,
                      float positionX);
};

}  // namespace rtype::client
