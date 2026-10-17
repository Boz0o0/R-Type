#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <optional>
#include <rtype/common/protocol/Packets.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <rtype/server/Game.hpp>
#include <rtype/server/GameEvents.hpp>
#include <rtype/server/GameServer.hpp>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <variant>
#include <vector>

namespace rtype::server {

using ampersand::net::Endpoint;

GameServer::GameServer(ServerNetwork& network, std::uint32_t seed)
    : _network(network), _game(seed) {}

void GameServer::run(const std::atomic<bool>& stopRequested) {
  auto nextTick = Clock::now();
  while (!stopRequested) {
    try {
      this->step(Clock::now());
    } catch (const std::exception& error) {
      std::cerr << "[server] tick " << this->_tick
                << " failed: " << error.what() << '\n';
    }
    nextTick += TICK_DURATION;
    const auto now = Clock::now();
    if (now - nextTick > MAX_LAG) {
      nextTick = now;
    }
    std::this_thread::sleep_until(nextTick);
  }
  this->shutdown();
}

void GameServer::step(Clock::time_point now) {
  this->handleMessages(now);
  this->dropSilentPlayers(now);
  const float deltaTime = std::chrono::duration<float>(TICK_DURATION).count();
  this->broadcastEvents(this->_game.update(deltaTime), now);
  ++this->_tick;
  if (this->_tick % SNAPSHOT_INTERVAL == 0) {
    this->broadcastSnapshot();
  }
  this->resendReliable(now);
}

std::uint32_t GameServer::tick() const noexcept { return this->_tick; }

std::size_t GameServer::playerCount() const noexcept {
  return static_cast<std::size_t>(std::ranges::count_if(
      this->_sessions,
      [](const auto& session) { return session.has_value(); }));
}

Game& GameServer::game() noexcept { return this->_game; }

void GameServer::handleMessages(Clock::time_point now) {
  for (const IncomingMessage& message : this->_network.receive()) {
    this->handleMessage(message, now);
  }
}

void GameServer::handleMessage(const IncomingMessage& message,
                               Clock::time_point now) {
  const protocol::Payload& payload = message.packet.payload;
  if (const auto* request = std::get_if<protocol::ConnectRequest>(&payload)) {
    this->acceptPlayer(message.from, request->playerName, now);
    return;
  }

  ClientSession* const session = this->findSession(message.from);
  if (session == nullptr) {
    return;  // only CONNECT_REQUEST is accepted from an unknown address
  }
  session->lastSeen = now;

  if (const auto* input = std::get_if<protocol::PlayerInput>(&payload)) {
    this->handleInput(*session, *input);
  } else if (const auto* ack = std::get_if<protocol::Ack>(&payload)) {
    session->outbox.acknowledge(ack->sequence);
  } else if (const auto* ping = std::get_if<protocol::Ping>(&payload)) {
    this->_network.send(protocol::Pong{.timestampMs = ping->timestampMs},
                        message.from);
  } else if (std::holds_alternative<protocol::Disconnect>(payload)) {
    this->removePlayer(session->playerId, protocol::DisconnectReason::Quit,
                       now);
  }
}

void GameServer::handleInput(ClientSession& session,
                             const protocol::PlayerInput& input) {
  if (session.lastInputTick && input.clientTick <= *session.lastInputTick) {
    return;
  }
  session.lastInputTick = input.clientTick;
  this->_game.setInput(session.playerId, input.inputMask);
}

void GameServer::acceptPlayer(const Endpoint& from, const std::string& name,
                              Clock::time_point now) {
  if (ClientSession* const known = this->findSession(from)) {
    known->lastSeen = now;
    this->sendTo(*known, protocol::ConnectAccept{.playerId = known->playerId},
                 now);
    return;
  }
  if (!isValidName(name)) {
    this->_network.send(
        protocol::ConnectReject{.reason = protocol::RejectReason::InvalidName},
        from);
    return;
  }

  for (std::uint8_t playerId = 0; playerId < protocol::MAX_PLAYERS;
       ++playerId) {
    auto& session = this->_sessions[playerId];
    if (session) {
      continue;
    }
    session = ClientSession{
        .playerId = playerId,
        .endpoint = from,
        .name = name,
        .lastSeen = now,
    };
    std::cout << "[server] player " << static_cast<int>(playerId) << " ("
              << name << ") joined from " << from.toString() << '\n';
    this->sendTo(*session, protocol::ConnectAccept{.playerId = playerId}, now);
    this->welcome(*session, now);
    for (auto& other : this->_sessions) {
      if (other && other->playerId != playerId) {
        this->sendTo(
            *other,
            protocol::PlayerJoined{.playerId = playerId, .playerName = name},
            now);
      }
    }
    this->_game.addPlayer(playerId);
    return;
  }
  this->_network.send(
      protocol::ConnectReject{.reason = protocol::RejectReason::ServerFull},
      from);
}

void GameServer::welcome(ClientSession& session, Clock::time_point now) {
  for (const auto& player : this->_sessions) {
    if (player) {
      this->sendTo(session,
                   protocol::PlayerJoined{
                       .playerId = player->playerId,
                       .playerName = player->name,
                   },
                   now);
    }
  }
  for (const EntityInfo& entity : this->_game.entities()) {
    this->sendTo(session, this->toPacket(EntitySpawned{.entity = entity}), now);
  }
  for (const PlayerChanged& state : this->_game.playerStates()) {
    this->sendTo(session, this->toPacket(state), now);
  }
  if (this->_game.phase() == Game::Phase::Playing) {
    this->sendTo(session, protocol::GameStart{.serverTick = this->_tick}, now);
  }
}

void GameServer::removePlayer(std::uint8_t playerId,
                              protocol::DisconnectReason reason,
                              Clock::time_point now) {
  auto& session = this->_sessions[playerId];
  if (!session) {
    return;
  }
  std::cout << "[server] player " << static_cast<int>(playerId) << " ("
            << session->name << ") left: " << toString(reason) << '\n';
  session.reset();
  this->_game.removePlayer(playerId);
  this->broadcast(protocol::PlayerLeft{.playerId = playerId, .reason = reason},
                  now);
}

void GameServer::dropSilentPlayers(Clock::time_point now) {
  for (std::uint8_t playerId = 0; playerId < protocol::MAX_PLAYERS;
       ++playerId) {
    const auto& session = this->_sessions[playerId];
    if (session && now - session->lastSeen > CLIENT_TIMEOUT) {
      this->removePlayer(playerId, protocol::DisconnectReason::Timeout, now);
    }
  }
}

void GameServer::resendReliable(Clock::time_point now) {
  for (std::uint8_t playerId = 0; playerId < protocol::MAX_PLAYERS;
       ++playerId) {
    auto& session = this->_sessions[playerId];
    if (!session) {
      continue;
    }
    const Endpoint target = session->endpoint;
    const bool acknowledged = session->outbox.resendDue(
        now, [this, &target](std::span<const std::byte> datagram) {
          this->_network.sendDatagram(datagram, target);
        });
    if (!acknowledged) {
      this->removePlayer(playerId, protocol::DisconnectReason::Timeout, now);
    }
  }
}

void GameServer::broadcastEvents(const std::vector<GameEvent>& events,
                                 Clock::time_point now) {
  for (const GameEvent& event : events) {
    if (std::holds_alternative<GameStarted>(event)) {
      std::cout << "[server] game started\n";
    } else if (const auto* ended = std::get_if<GameEnded>(&event)) {
      std::cout << "[server] game over: "
                << (ended->victory ? "victory" : "defeat") << ", score "
                << ended->score << '\n';
    }
    this->broadcast(this->toPacket(event), now);
  }
}

void GameServer::broadcastSnapshot() {
  const std::vector<EntityInfo> entities = this->_game.entities();
  // A big world is split into several snapshots of the same tick.
  std::size_t first = 0;
  do {
    const std::size_t last =
        std::min(first + protocol::MAX_SNAPSHOT_ENTITIES, entities.size());
    protocol::WorldSnapshot snapshot{.serverTick = this->_tick, .entities = {}};
    for (std::size_t index = first; index < last; ++index) {
      const EntityInfo& entity = entities[index];
      snapshot.entities.push_back({
          .entityId = entity.id,
          .x = entity.position.x,
          .y = entity.position.y,
          .velocityX = entity.velocity.x,
          .velocityY = entity.velocity.y,
      });
    }
    const OutgoingDatagram datagram = this->_network.encode(snapshot);
    for (const auto& session : this->_sessions) {
      if (session) {
        this->_network.sendDatagram(datagram.bytes, session->endpoint);
      }
    }
    first = last;
  } while (first < entities.size());
}

void GameServer::shutdown() {
  for (std::uint8_t playerId = 0; playerId < protocol::MAX_PLAYERS;
       ++playerId) {
    auto& session = this->_sessions[playerId];
    if (!session) {
      continue;
    }
    for (std::uint32_t attempt = 0; attempt < DISCONNECT_REPEAT; ++attempt) {
      this->_network.send(
          protocol::Disconnect{
              .reason = protocol::DisconnectReason::ServerShutdown,
          },
          session->endpoint);
    }
    std::cout << "[server] player " << static_cast<int>(playerId) << " ("
              << session->name << ") left: "
              << toString(protocol::DisconnectReason::ServerShutdown) << '\n';
    session.reset();
    this->_game.removePlayer(playerId);
  }
}

void GameServer::sendTo(ClientSession& session,
                        const protocol::Payload& payload,
                        Clock::time_point now) {
  if (!protocol::isReliable(protocol::typeOf(payload))) {
    this->_network.send(payload, session.endpoint);
    return;
  }
  OutgoingDatagram datagram = this->_network.encode(payload);
  this->_network.sendDatagram(datagram.bytes, session.endpoint);
  session.outbox.add(datagram.sequence, std::move(datagram.bytes), now);
}

void GameServer::broadcast(const protocol::Payload& payload,
                           Clock::time_point now) {
  for (auto& session : this->_sessions) {
    if (session) {
      this->sendTo(*session, payload, now);
    }
  }
}

protocol::Payload GameServer::toPacket(const GameEvent& event) const {
  if (const auto* spawned = std::get_if<EntitySpawned>(&event)) {
    const EntityInfo& entity = spawned->entity;
    return protocol::EntitySpawn{
        .entityId = entity.id,
        .entityType = entity.kind,
        .ownerId = entity.ownerId,
        .x = entity.position.x,
        .y = entity.position.y,
        .velocityX = entity.velocity.x,
        .velocityY = entity.velocity.y,
    };
  }
  if (const auto* destroyed = std::get_if<EntityDestroyed>(&event)) {
    return protocol::EntityDestroy{
        .entityId = destroyed->id,
        .reason = destroyed->reason,
    };
  }
  if (const auto* changed = std::get_if<PlayerChanged>(&event)) {
    return protocol::PlayerState{
        .playerId = changed->playerId,
        .health = changed->health,
        .lives = changed->lives,
        .score = changed->score,
    };
  }
  if (const auto* ended = std::get_if<GameEnded>(&event)) {
    return protocol::GameOver{
        .result = ended->victory ? protocol::GameResult::Victory
                                 : protocol::GameResult::Defeat,
        .finalScore = ended->score,
    };
  }
  return protocol::GameStart{.serverTick = this->_tick};
}

ClientSession* GameServer::findSession(const Endpoint& endpoint) {
  for (auto& session : this->_sessions) {
    if (session && session->endpoint == endpoint) {
      return &*session;
    }
  }
  return nullptr;
}

bool GameServer::isValidName(std::string_view name) {
  if (name.empty()) {
    return false;
  }
  while (!name.empty()) {
    const std::size_t length = characterLength(name);
    if (length == 0) {
      return false;
    }
    name.remove_prefix(length);
  }
  return true;
}

std::size_t GameServer::characterLength(std::string_view text) {
  const auto lead = static_cast<unsigned char>(text.front());
  if (lead < 0x20 || lead == 0x7F) {
    return 0;  // control character
  }
  if (lead < 0x80) {
    return 1;
  }
  // Length of the sequence, and range of its second byte: this rules out
  // overlong forms, surrogates and code points past U+10FFFF.
  std::size_t length = 2;
  unsigned char low = 0x80;
  unsigned char high = 0xBF;
  if (lead >= 0xE0 && lead <= 0xEF) {
    length = 3;
    low = lead == 0xE0 ? 0xA0 : 0x80;
    high = lead == 0xED ? 0x9F : 0xBF;
  } else if (lead >= 0xF0 && lead <= 0xF4) {
    length = 4;
    low = lead == 0xF0 ? 0x90 : 0x80;
    high = lead == 0xF4 ? 0x8F : 0xBF;
  } else if (lead < 0xC2 || lead > 0xDF) {
    return 0;
  }
  if (text.size() < length) {
    return 0;
  }
  const auto second = static_cast<unsigned char>(text[1]);
  if (second < low || second > high) {
    return 0;
  }
  for (std::size_t index = 2; index < length; ++index) {
    const auto next = static_cast<unsigned char>(text[index]);
    if (next < 0x80 || next > 0xBF) {
      return 0;
    }
  }
  return length;
}

std::string_view GameServer::toString(protocol::DisconnectReason reason) {
  switch (reason) {
    case protocol::DisconnectReason::Quit:
      return "quit";
    case protocol::DisconnectReason::Timeout:
      return "timeout";
    case protocol::DisconnectReason::Kicked:
      return "kicked";
    case protocol::DisconnectReason::ServerShutdown:
      return "server shutdown";
  }
  return "unknown";
}

}  // namespace rtype::server
