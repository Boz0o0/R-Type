#include <ampersand/core/resources/Clock.hpp>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <rtype/common/protocol/Packets.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <rtype/server/GameServer.hpp>
#include <string>
#include <string_view>
#include <thread>
#include <variant>

namespace rtype::server {

using ampersand::net::Endpoint;

GameServer::GameServer(ServerNetwork& network) : _network(network) {
  this->_world.addResource<ampersand::core::Clock>();
}

void GameServer::run(const std::atomic<bool>& stopRequested) {
  auto nextTick = Clock::now();
  while (!stopRequested) {
    this->step(Clock::now());
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
  this->update();
  ++this->_tick;
  if (this->_tick % SNAPSHOT_INTERVAL == 0) {
    this->broadcastSnapshot();
  }
}

std::uint32_t GameServer::tick() const noexcept { return this->_tick; }

std::size_t GameServer::playerCount() const noexcept {
  std::size_t count = 0;
  for (const auto& session : this->_sessions) {
    if (session) {
      ++count;
    }
  }
  return count;
}

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

  if (const auto* ping = std::get_if<protocol::Ping>(&payload)) {
    this->_network.send(protocol::Pong{.timestampMs = ping->timestampMs},
                        message.from);
  } else if (std::holds_alternative<protocol::Disconnect>(payload)) {
    this->removePlayer(session->playerId, "quit");
  }
  // PLAYER_INPUT and ACK only keep the client alive for now, and packets
  // meant for clients are ignored.
}

void GameServer::acceptPlayer(const Endpoint& from, const std::string& name,
                              Clock::time_point now) {
  // Same address again: the CONNECT_ACCEPT was lost, send it again.
  if (ClientSession* const known = this->findSession(from)) {
    known->lastSeen = now;
    this->_network.send(protocol::ConnectAccept{.playerId = known->playerId},
                        from);
    return;
  }

  for (std::uint8_t playerId = 0; playerId < protocol::MAX_PLAYERS;
       ++playerId) {
    auto& session = this->_sessions[playerId];
    if (!session) {
      session = ClientSession{
          .playerId = playerId,
          .endpoint = from,
          .name = name,
          .lastSeen = now,
      };
      this->_network.send(protocol::ConnectAccept{.playerId = playerId}, from);
      std::cout << "[server] player " << static_cast<int>(playerId) << " ("
                << name << ") joined from " << from.toString() << '\n';
      return;
    }
  }
  this->_network.send(
      protocol::ConnectReject{.reason = protocol::RejectReason::ServerFull},
      from);
}

void GameServer::removePlayer(std::uint8_t playerId, std::string_view reason) {
  auto& session = this->_sessions[playerId];
  if (!session) {
    return;
  }
  std::cout << "[server] player " << static_cast<int>(playerId) << " ("
            << session->name << ") left: " << reason << '\n';
  session.reset();
}

void GameServer::dropSilentPlayers(Clock::time_point now) {
  for (std::uint8_t playerId = 0; playerId < protocol::MAX_PLAYERS;
       ++playerId) {
    const auto& session = this->_sessions[playerId];
    if (session && now - session->lastSeen > CLIENT_TIMEOUT) {
      this->removePlayer(playerId, "timeout");
    }
  }
}

void GameServer::update() {
  auto& clock = this->_world.getResource<ampersand::core::Clock>();
  clock.dt = std::chrono::duration<float>(TICK_DURATION).count();
  clock.elapsed += clock.dt;
  this->_movementSystem.update(this->_world);
}

void GameServer::broadcastSnapshot() {
  // The entities are added once the game spawns some; for now the snapshot
  // only tells the clients that the server is ticking.
  const protocol::WorldSnapshot snapshot{
      .serverTick = this->_tick,
      .entities = {},
  };
  for (const auto& session : this->_sessions) {
    if (session) {
      this->_network.send(snapshot, session->endpoint);
    }
  }
}

void GameServer::shutdown() {
  for (std::uint8_t playerId = 0; playerId < protocol::MAX_PLAYERS;
       ++playerId) {
    if (const auto& session = this->_sessions[playerId]) {
      this->_network.send(
          protocol::Disconnect{
              .reason = protocol::DisconnectReason::ServerShutdown,
          },
          session->endpoint);
      this->removePlayer(playerId, "server shutdown");
    }
  }
}

ClientSession* GameServer::findSession(const Endpoint& endpoint) {
  for (auto& session : this->_sessions) {
    if (session && session->endpoint == endpoint) {
      return &*session;
    }
  }
  return nullptr;
}

}  // namespace rtype::server
