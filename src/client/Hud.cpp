#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/render/FontId.hpp>
#include <ampersand/render/components/Text.hpp>
#include <cstdint>
#include <optional>
#include <rtype/client/Hud.hpp>
#include <rtype/client/ServerConnection.hpp>
#include <rtype/common/protocol/PacketCodec.hpp>
#include <rtype/common/protocol/Packets.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <string>
#include <utility>
#include <variant>

namespace rtype::client {

namespace core = ampersand::core;
namespace render = ampersand::render;

using State = ServerConnection::State;

Hud::Hud(core::ecs::World& world, render::FontId font)
    : _world(world),
      _status(this->addText(font, {MARGIN, MARGIN}, TEXT_SIZE)),
      _stats(this->addText(font, {STATS_X, MARGIN}, TEXT_SIZE)),
      _event(this->addText(font, {MARGIN, EVENT_Y}, TEXT_SIZE)),
      _banner(this->addText(font, {0.0F, BANNER_Y}, BANNER_SIZE)) {
  this->_world.get<render::Text>(this->_banner).color = BANNER_COLOR;
}

void Hud::showConnection(State state, std::optional<std::uint8_t> playerId,
                         std::optional<protocol::RejectReason> rejectReason) {
  switch (state) {
    case State::Connecting:
      this->setText(this->_status, "Connecting...");
      break;
    case State::Connected: {
      const std::uint8_t player = playerId.value_or(0);
      this->_playerId = player;
      this->setText(this->_status, "You: " + playerLabel(player));
      this->_world.get<render::Text>(this->_status).color =
          PLAYER_COLORS[player % protocol::MAX_PLAYERS];
      break;
    }
    case State::Rejected:
      this->setBanner(rejectText(rejectReason));
      break;
    case State::Unreachable:
      this->setBanner("Server unreachable");
      break;
    case State::Disconnected:
      this->setBanner("Disconnected");
      break;
  }
}

void Hud::apply(const protocol::Payload& packet) {
  if (const auto* joined = std::get_if<protocol::PlayerJoined>(&packet)) {
    this->setText(this->_event, playerLabel(joined->playerId) + " " +
                                    joined->playerName + " joined");
  } else if (const auto* left = std::get_if<protocol::PlayerLeft>(&packet)) {
    this->setText(this->_event,
                  playerLabel(left->playerId) + leftText(left->reason));
  } else if (const auto* player = std::get_if<protocol::PlayerState>(&packet)) {
    if (player->playerId == this->_playerId) {
      this->setText(this->_stats,
                    "HP " + std::to_string(player->health) + "   Lives " +
                        std::to_string(player->lives) + "   Score " +
                        std::to_string(player->score));
    }
  } else if (const auto* over = std::get_if<protocol::GameOver>(&packet)) {
    const std::string result =
        over->result == protocol::GameResult::Victory ? "VICTORY" : "GAME OVER";
    this->setBanner(result + "   Score " + std::to_string(over->finalScore));
    this->_gameOver = true;
  }
}

const std::string& Hud::status() const {
  return this->_world.get<render::Text>(this->_status).value;
}

const std::string& Hud::stats() const {
  return this->_world.get<render::Text>(this->_stats).value;
}

const std::string& Hud::event() const {
  return this->_world.get<render::Text>(this->_event).value;
}

const std::string& Hud::banner() const {
  return this->_world.get<render::Text>(this->_banner).value;
}

std::string Hud::playerLabel(std::uint8_t playerId) {
  return "P" + std::to_string(playerId + 1);
}

std::string Hud::rejectText(std::optional<protocol::RejectReason> reason) {
  if (!reason) {
    return "Refused by the server";
  }
  switch (*reason) {
    case protocol::RejectReason::ServerFull:
      return "Server full";
    case protocol::RejectReason::GameInProgress:
      return "Game already started";
    case protocol::RejectReason::VersionMismatch:
      return "Wrong game version";
    case protocol::RejectReason::InvalidName:
      return "Invalid player name";
  }
  return "Refused by the server";
}

std::string Hud::leftText(protocol::DisconnectReason reason) {
  switch (reason) {
    case protocol::DisconnectReason::Timeout:
      return " lost connection";
    case protocol::DisconnectReason::Kicked:
      return " was kicked";
    case protocol::DisconnectReason::Quit:
    case protocol::DisconnectReason::ServerShutdown:
      break;
  }
  return " left";
}

core::ecs::Entity Hud::addText(render::FontId font, core::math::Vec2f position,
                               unsigned int size) {
  const core::ecs::Entity entity = this->_world.create();
  this->_world.add(entity, core::Transform{
                               .position = {position.x, position.y, HUD_LAYER},
                           });
  this->_world.add(entity, render::Text{.font = font, .size = size});
  return entity;
}

void Hud::setText(core::ecs::Entity entity, std::string value) {
  this->_world.get<render::Text>(entity).value = std::move(value);
}

void Hud::setBanner(std::string value) {
  if (this->_gameOver) {
    return;
  }
  this->_world.get<core::Transform>(this->_banner).position.x =
      SCREEN_CENTER_X - (static_cast<float>(value.size()) *
                         static_cast<float>(BANNER_SIZE) * CHAR_WIDTH / 2.0F);
  this->setText(this->_banner, std::move(value));
}

}  // namespace rtype::client
