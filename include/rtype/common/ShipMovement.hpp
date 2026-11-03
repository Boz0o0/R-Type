#pragma once

#include <ampersand/core/math/Vec2.hpp>
#include <cstdint>

namespace rtype::common {

/**
 * @brief How a player's ship moves, shared so the client can predict its own
 * ship exactly like the server moves it.
 */
class ShipMovement {
 public:
  /** @brief World units per second. */
  static constexpr float SPEED = 450.0F;
  /** @brief Size of the ship's hitbox. */
  static constexpr ampersand::core::math::Vec2f SIZE{64.0F, 32.0F};
  /** @brief Size of the screen the ship stays in. */
  static constexpr ampersand::core::math::Vec2f SCREEN{1920.0F, 1080.0F};

  /** @brief Velocity for the InputFlag bits held, the same on diagonals. */
  [[nodiscard]] static ampersand::core::math::Vec2f velocity(
      std::uint8_t inputMask);
  /** @brief Top-left corner moved back inside the screen if needed. */
  [[nodiscard]] static ampersand::core::math::Vec2f keepOnScreen(
      ampersand::core::math::Vec2f position);

 private:
  /** @brief Speed factor on each axis when moving diagonally (1 / sqrt 2). */
  static constexpr float DIAGONAL_FACTOR = 0.70710678F;
};

}  // namespace rtype::common
