#pragma once

#include <ampersand/core/ecs/World.hpp>
#include <ampersand/render/TextureId.hpp>
#include <array>
#include <cstddef>
#include <cstdint>

namespace rtype::client {

/**
 * @brief The scrolling background: layers of stars moving left at their own
 *        speed, each drawn twice side by side so that it loops.
 */
class Starfield {
 public:
  /** @brief Number of layers, from the farthest to the nearest. */
  static constexpr std::size_t LAYER_COUNT = 2;
  /** @brief Image of each layer, from the farthest to the nearest. */
  using Textures = std::array<ampersand::render::TextureId, LAYER_COUNT>;

  /**
   * @brief Creates the entities of the background.
   *
   * @param world The world to create them in.
   * @param textures Image of each layer.
   */
  static void spawn(ampersand::core::ecs::World& world,
                    const Textures& textures);

 private:
  /** @brief How a layer moves and where it is drawn. */
  struct Layer {
    /** @brief Speed to the left, in world units per second. */
    float speed{};
    /** @brief Sprite layer, under the game. */
    std::int32_t depth{};
  };

  /** @brief Width of a copy of a layer: the images are 960x540, drawn twice
   *         as big to cover the 1920x1080 world. */
  static constexpr float WIDTH = 1920.0F;
  /** @brief Scale of the images. */
  static constexpr float SCALE = 2.0F;
  /** @brief The layers, from the farthest to the nearest. */
  static constexpr std::array<Layer, LAYER_COUNT> LAYERS{{
      {.speed = 30.0F, .depth = -20},
      {.speed = 90.0F, .depth = -10},
  }};

  /**
   * @brief Creates one copy of a layer.
   *
   * @param world The world to create it in.
   * @param texture Image of the layer.
   * @param layer Speed and depth of the layer.
   * @param positionX X of its left edge.
   */
  static void addCopy(ampersand::core::ecs::World& world,
                      ampersand::render::TextureId texture, const Layer& layer,
                      float positionX);
};

}  // namespace rtype::client
