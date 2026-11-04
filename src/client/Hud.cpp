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
#include <rtype/client/TextEntity.hpp>
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
      _status(createText(world, font, {MARGIN, MARGIN}, TEXT_SIZE)),
      _stats(createText(world, font, {STATS_X, MARGIN}, TEXT_SIZE)),
      _event(createText(world, font, {MARGIN, EVENT_Y}, TEXT_SIZE)),
      _banner(createText(world, font, {0.0F, BANNER_Y}, BANNER_SIZE)) {
  this->_world.get<render::Text>(this->_banner).color = BANNER_COLOR;
}

void Hud::showConnection(State state,
                         const std::optional<protocol::RoomJoined>& room,
                         std::optional<protocol::RejectReason> rejectReason) {
  const bool enteringLobby =
      isLobby(state) && (!this->_shownState || !isLobby(*this->_shownState));
  this->_shownState = state;
  switch (state) {
    case State::LoggedOut:
      // The login menu shows what happened.
      this->setText(this->_status, {});
      break;
    case State::Connecting:
      this->setText(this->_status, "Connecting...");
      break;
    case State::InLobby:
    case State::Joining:
    case State::Leaving:
      if (enteringLobby) {
        this->clear();
      }
      break;
    case State::InRoom: {
      const std::uint8_t player = room ? room->playerId : 0;
      this->_playerId = player;
      std::string status = "You: " + playerLabel(player);
      if (room) {
        status += "   " + roomLabel(*room);
      }
      this->setText(this->_status, std::move(status));
      this->_world.get<render::Text>(this->_status).color =
          PLAYER_COLORS[player % protocol::MAX_PLAYERS];
      break;
    }
    case State::Rejected:
      this->setBanner(rejectText(rejectReason));
      this->setText(this->_event, QUIT_HINT);
      break;
    case State::Unreachable:
      this->setBanner("Server unreachable");
      this->setText(this->_event, QUIT_HINT);
      break;
    case State::Disconnected:
      this->setBanner("Disconnected");
      this->setText(this->_event, QUIT_HINT);
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
  } else if (std::holds_alternative<protocol::GameStart>(packet)) {
    this->_gameOver = false;
    this->setBanner("");
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

std::string Hud::roomLabel(const protocol::RoomJoined& room) {
  if (!room.code.empty()) {
    return "Code " + room.code;
  }
  return "Room " + std::to_string(room.roomId);
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
    case protocol::RejectReason::PartyFull:
      return "Room full";
    case protocol::RejectReason::InvalidPartyCode:
      return "Invalid room code";
    case protocol::RejectReason::RoomNotFound:
      return "Room closed";
    case protocol::RejectReason::BadCredentials:
      return "Wrong username or password";
    case protocol::RejectReason::AccountExists:
      return "Username already taken";
    case protocol::RejectReason::AlreadyLoggedIn:
      return "Account already in use";
    case protocol::RejectReason::InvalidPassword:
      return "Invalid password";
    case protocol::RejectReason::TooManyAttempts:
      return "Too many attempts, wait a minute";
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

bool Hud::isLobby(State state) {
  return state == State::InLobby || state == State::Joining ||
         state == State::Leaving;
}

void Hud::clear() {
  this->_gameOver = false;
  this->_playerId.reset();
  for (const core::ecs::Entity entity :
       {this->_status, this->_stats, this->_event, this->_banner}) {
    this->setText(entity, {});
  }
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
                         static_cast<float>(BANNER_SIZE) * LETTER_WIDTH / 2.0F);
  this->setText(this->_banner, std::move(value));
}

}  // namespace rtype::client
