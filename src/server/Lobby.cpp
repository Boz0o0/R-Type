#include <ampersand/net/UdpSocket.hpp>
#include <atomic>
#include <cstddef>
#include <iostream>
#include <memory>
#include <optional>
#include <random>
#include <rtype/common/protocol/PacketCodec.hpp>
#include <rtype/common/protocol/Packets.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <rtype/server/GameServer.hpp>
#include <rtype/server/Lobby.hpp>
#include <rtype/server/Log.hpp>
#include <rtype/server/ServerNetwork.hpp>
#include <stop_token>
#include <thread>
#include <utility>
#include <variant>

namespace rtype::server {

using ampersand::net::Endpoint;

Lobby::Lobby(ServerNetwork& network, std::size_t maxParties)
    : _network(network), _maxParties(maxParties) {}

void Lobby::run(const std::atomic<bool>& stopRequested) {
  while (!stopRequested) {
    this->step();
    std::this_thread::sleep_for(IDLE_DELAY);
  }
  for (auto& [partyId, party] : this->_parties) {
    party.thread.request_stop();
  }
  this->_parties.clear();
  this->_routes.clear();
}

void Lobby::step() {
  this->applyChanges();
  for (IncomingMessage& message : this->_network.receive()) {
    this->route(std::move(message));
  }
  this->closeEmptyParties();
}

std::size_t Lobby::partyCount() const noexcept { return this->_parties.size(); }

std::size_t Lobby::playerCount() const noexcept { return this->_routes.size(); }

std::optional<Lobby::PartyId> Lobby::partyOf(const Endpoint& endpoint) const {
  const auto found = this->_routes.find(endpoint.toString());
  if (found == this->_routes.end()) {
    return std::nullopt;
  }
  return found->second;
}

void Lobby::applyChanges() {
  for (const Change& change : this->_changes.drain()) {
    const auto party = this->_parties.find(change.party);
    if (party == this->_parties.end()) {
      continue;  // closed since
    }
    if (change.change == SessionChange::Left) {
      this->unbind(change.endpoint, change.party);
      continue;
    }
    const auto route = this->_routes.find(change.endpoint.toString());
    if (route == this->_routes.end()) {
      // It left and joined again before the Lobby saw the leave.
      this->bind(change.endpoint, change.party);
    } else if (route->second != change.party) {
      // An old CONNECT_REQUEST let it join this party after it moved to
      // another one: it cannot play in both.
      protocol::Packet quit;
      quit.payload =
          protocol::Disconnect{.reason = protocol::DisconnectReason::Quit};
      party->second.server->push({.from = change.endpoint, .packet = quit});
    }
  }
}

void Lobby::route(IncomingMessage message) {
  const auto found = this->_routes.find(message.from.toString());
  if (found == this->_routes.end()) {
    this->connect(std::move(message));
    return;
  }
  const auto party = this->_parties.find(found->second);
  if (party != this->_parties.end() &&
      !party->second.server->push(std::move(message))) {
    logLine(std::cerr, "[server] party ", found->second,
            " is overloaded, dropped a packet from ", found->first);
  }
}

void Lobby::connect(IncomingMessage message) {
  const auto* request =
      std::get_if<protocol::ConnectRequest>(&message.packet.payload);
  if (request == nullptr) {
    return;  // only CONNECT_REQUEST is accepted from an unknown address
  }
  // Checked here too, so that bad names never open a party.
  if (!GameServer::isValidName(request->playerName)) {
    this->_network.send(
        protocol::ConnectReject{.reason = protocol::RejectReason::InvalidName},
        message.from);
    return;
  }
  const std::optional<PartyId> party = this->place();
  if (!party) {
    this->_network.send(
        protocol::ConnectReject{.reason = protocol::RejectReason::ServerFull},
        message.from);
    return;
  }
  const Endpoint from = message.from;
  this->bind(from, *party);
  if (!this->_parties.at(*party).server->push(std::move(message))) {
    this->unbind(from, *party);
  }
}

std::optional<Lobby::PartyId> Lobby::place() {
  for (const auto& [partyId, party] : this->_parties) {
    if (party.players < protocol::MAX_PLAYERS) {
      return partyId;
    }
  }
  if (this->_parties.size() >= this->_maxParties) {
    return std::nullopt;
  }
  return this->open();
}

Lobby::PartyId Lobby::open() {
  const PartyId partyId = this->_nextPartyId++;
  auto server = std::make_unique<GameServer>(
      this->_network, std::random_device{}(),
      PartyLink{
          .id = partyId,
          .listener =
              [this, partyId](const Endpoint& endpoint, SessionChange change) {
                if (!this->_changes.push({
                        .party = partyId,
                        .endpoint = endpoint,
                        .change = change,
                    })) {
                  logLine(std::cerr,
                          "[server] lobby overloaded, lost a change of ",
                          endpoint.toString());
                }
              },
      });
  GameServer& game = *server;
  this->_parties.emplace(
      partyId,
      Party{
          .server = std::move(server),
          .players = 0,
          .thread = std::jthread(
              [&game](const std::stop_token& token) { game.run(token); }),
      });
  logLine(std::cout, "[server] party ", partyId, " opened");
  return partyId;
}

void Lobby::closeEmptyParties() {
  for (auto party = this->_parties.begin(); party != this->_parties.end();) {
    if (party->second.players != 0) {
      ++party;
      continue;
    }
    logLine(std::cout, "[server] party ", party->first, " closed");
    party = this->_parties.erase(party);
  }
}

void Lobby::bind(const Endpoint& endpoint, PartyId party) {
  if (this->_routes.emplace(endpoint.toString(), party).second) {
    ++this->_parties.at(party).players;
  }
}

void Lobby::unbind(const Endpoint& endpoint, PartyId party) {
  const auto found = this->_routes.find(endpoint.toString());
  if (found == this->_routes.end() || found->second != party) {
    return;  // routed to another party since
  }
  this->_routes.erase(found);
  --this->_parties.at(party).players;
}

}  // namespace rtype::server
