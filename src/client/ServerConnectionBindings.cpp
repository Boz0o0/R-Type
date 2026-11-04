#include <ampersand/core/ecs/EventQueue.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/scripting/systems/ScriptingSystem.hpp>
#include <cstdint>
#include <optional>
#include <rtype/client/ServerConnection.hpp>
#include <rtype/common/protocol/PacketCodec.hpp>
#include <rtype/common/protocol/Packets.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <string>
#include <utility>
#include <variant>

namespace rtype::client {

namespace {

/**
 * @brief The packets the scripts show, as tables with a `type` field. The
 *        others (spawns, snapshots...) are for WorldMirror only.
 */
std::optional<sol::table> toLua(sol::state& lua,
                                const protocol::Payload& packet) {
  if (const auto* joined = std::get_if<protocol::PlayerJoined>(&packet)) {
    return lua.create_table_with("type", "PlayerJoined", "playerId",
                                 joined->playerId, "playerName",
                                 joined->playerName);
  }
  if (const auto* left = std::get_if<protocol::PlayerLeft>(&packet)) {
    return lua.create_table_with("type", "PlayerLeft", "playerId",
                                 left->playerId, "reason", left->reason);
  }
  if (const auto* player = std::get_if<protocol::PlayerState>(&packet)) {
    return lua.create_table_with(
        "type", "PlayerState", "playerId", player->playerId, "health",
        player->health, "lives", player->lives, "score", player->score);
  }
  if (const auto* over = std::get_if<protocol::GameOver>(&packet)) {
    return lua.create_table_with("type", "GameOver", "result", over->result,
                                 "finalScore", over->finalScore);
  }
  if (std::holds_alternative<protocol::GameStart>(packet)) {
    return lua.create_table_with("type", "GameStart");
  }
  return std::nullopt;
}

}  // namespace

void ServerConnection::bindScripting(
    ampersand::scripting::ScriptingSystem& scripting,
    ampersand::core::ecs::World& world) {
  sol::state& lua = scripting.lua();
  sol::table server = lua.create_named_table("server");

  server.new_enum("State", "LoggedOut", State::LoggedOut, "Connecting",
                  State::Connecting, "InLobby", State::InLobby, "Joining",
                  State::Joining, "InRoom", State::InRoom, "Leaving",
                  State::Leaving, "Rejected", State::Rejected, "Unreachable",
                  State::Unreachable, "Disconnected", State::Disconnected);
  server.new_enum("RejectReason", "ServerFull",
                  protocol::RejectReason::ServerFull, "GameInProgress",
                  protocol::RejectReason::GameInProgress, "VersionMismatch",
                  protocol::RejectReason::VersionMismatch, "InvalidName",
                  protocol::RejectReason::InvalidName, "PartyFull",
                  protocol::RejectReason::PartyFull, "InvalidPartyCode",
                  protocol::RejectReason::InvalidPartyCode, "RoomNotFound",
                  protocol::RejectReason::RoomNotFound, "BadCredentials",
                  protocol::RejectReason::BadCredentials, "AccountExists",
                  protocol::RejectReason::AccountExists, "AlreadyLoggedIn",
                  protocol::RejectReason::AlreadyLoggedIn, "InvalidPassword",
                  protocol::RejectReason::InvalidPassword, "TooManyAttempts",
                  protocol::RejectReason::TooManyAttempts);
  server.new_enum("DisconnectReason", "Quit", protocol::DisconnectReason::Quit,
                  "Timeout", protocol::DisconnectReason::Timeout, "Kicked",
                  protocol::DisconnectReason::Kicked, "ServerShutdown",
                  protocol::DisconnectReason::ServerShutdown);
  server.new_enum("GameResult", "Victory", protocol::GameResult::Victory,
                  "Defeat", protocol::GameResult::Defeat);
  server["MAX_PLAYERS"] = protocol::MAX_PLAYERS;
  server["MAX_PLAYER_NAME_LENGTH"] = protocol::MAX_PLAYER_NAME_LENGTH;
  server["MAX_PASSWORD_LENGTH"] = protocol::MAX_PASSWORD_LENGTH;
  server["MAX_PARTY_CODE_LENGTH"] = protocol::MAX_PARTY_CODE_LENGTH;

  server.set_function("state", [this] { return this->state(); });
  server.set_function("rejectReason", [this] { return this->rejectReason(); });
  server.set_function("roomRejectReason",
                      [this] { return this->roomRejectReason(); });
  server.set_function("room", [this, &lua]() -> sol::object {
    if (!this->room()) {
      return sol::lua_nil;
    }
    return lua.create_table_with("roomId", this->room()->roomId, "playerId",
                                 this->room()->playerId, "code",
                                 this->room()->code);
  });
  server.set_function("rooms", [this, &lua] {
    sol::table rooms = lua.create_table();
    for (const protocol::RoomInfo& room : this->rooms()) {
      rooms.add(lua.create_table_with("roomId", room.roomId, "players",
                                      room.players));
    }
    return rooms;
  });
  server.set_function(
      "logIn",
      [this](std::string username, std::string password, bool createAccount) {
        this->logIn(std::move(username), std::move(password), createAccount);
      });
  server.set_function(
      "joinRoom", [this](std::uint32_t roomId, std::string code) {
        this->enterRoom(
            protocol::RoomJoin{.roomId = roomId, .code = std::move(code)});
      });
  server.set_function("createRoom", [this](bool isPrivate) {
    this->enterRoom(protocol::RoomCreate{
        .visibility = isPrivate ? protocol::RoomVisibility::Private
                                : protocol::RoomVisibility::Public,
    });
  });
  server.set_function("packets", [&world, &lua] {
    sol::table packets = lua.create_table();
    for (const protocol::Payload& packet :
         world
             .getResource<ampersand::core::ecs::EventQueue<protocol::Payload>>()
             .read()) {
      if (const auto value = toLua(lua, packet)) {
        packets.add(*value);
      }
    }
    return packets;
  });
}

}  // namespace rtype::client
