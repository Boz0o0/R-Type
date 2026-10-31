#pragma once

#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/render/Color.hpp>
#include <ampersand/render/FontId.hpp>
#include <array>
#include <cstdint>
#include <optional>
#include <rtype/client/ServerConnection.hpp>
#include <rtype/common/protocol/PacketCodec.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <string>

namespace rtype::client {

class Hud {
 public:
  static constexpr std::array<ampersand::render::Color, protocol::MAX_PLAYERS>
      PLAYER_COLORS{{
          {.r = 90, .g = 220, .b = 255},
          {.r = 255, .g = 110, .b = 255},
          {.r = 110, .g = 255, .b = 110},
          {.r = 255, .g = 110, .b = 110},
      }};

  Hud(ampersand::core::ecs::World& world, ampersand::render::FontId font);

  void showConnection(ServerConnection::State state,
                      std::optional<std::uint8_t> playerId,
                      std::optional<protocol::RejectReason> rejectReason);

  void apply(const protocol::Payload& packet);

  [[nodiscard]] const std::string& status() const;
  [[nodiscard]] const std::string& stats() const;
  [[nodiscard]] const std::string& event() const;
  [[nodiscard]] const std::string& banner() const;

 private:
  static constexpr unsigned int TEXT_SIZE = 36;
  static constexpr unsigned int BANNER_SIZE = 80;
  static constexpr std::int32_t HUD_LAYER = 10;
  static constexpr float MARGIN = 30.0F;
  static constexpr float STATS_X = 1300.0F;
  static constexpr float EVENT_Y = 1020.0F;
  static constexpr float BANNER_Y = 480.0F;
  static constexpr float SCREEN_CENTER_X = 960.0F;
  static constexpr ampersand::render::Color BANNER_COLOR{
      .r = 255, .g = 220, .b = 80};
  static constexpr float LETTER_WIDTH = 0.55F;
  static constexpr const char* QUIT_HINT = "Press any key to quit";

  [[nodiscard]] static std::string playerLabel(std::uint8_t playerId);
  [[nodiscard]] static std::string rejectText(
      std::optional<protocol::RejectReason> reason);
  [[nodiscard]] static std::string leftText(protocol::DisconnectReason reason);

  ampersand::core::ecs::Entity addText(ampersand::render::FontId font,
                                       ampersand::core::math::Vec2f position,
                                       unsigned int size);
  void setText(ampersand::core::ecs::Entity entity, std::string value);
  void setBanner(std::string value);

  ampersand::core::ecs::World& _world;
  ampersand::core::ecs::Entity _status;
  ampersand::core::ecs::Entity _stats;
  ampersand::core::ecs::Entity _event;
  ampersand::core::ecs::Entity _banner;
  std::optional<std::uint8_t> _playerId;
  bool _gameOver{false};
};

}  // namespace rtype::client
