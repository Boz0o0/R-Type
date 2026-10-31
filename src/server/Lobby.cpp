#include <ampersand/net/UdpSocket.hpp>
#include <atomic>
#include <cstddef>
#include <cstdint>
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
#include <string>
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
  // Each party tells its own clients, then its thread is joined.
  this->_parties.clear();
  this->sayGoodbyeToVisitors();
  this->_clients.clear();
  this->_codes.clear();
  this->_players = 0;
}

void Lobby::step() { this->step(Clock::now()); }

void Lobby::step(Clock::time_point now) {
  this->applyChanges(now);
  for (IncomingMessage& message : this->_network.receive()) {
    this->route(std::move(message), now);
  }
  this->dropSilentVisitors(now);
  this->closeEmptyParties();
}

std::size_t Lobby::partyCount() const noexcept { return this->_parties.size(); }

std::size_t Lobby::playerCount() const noexcept { return this->_players; }

std::size_t Lobby::visitorCount() const noexcept {
  return this->_clients.size() - this->_players;
}

std::optional<Lobby::PartyId> Lobby::partyOf(const Endpoint& endpoint) const {
  const auto found = this->_clients.find(endpoint.toString());
  if (found == this->_clients.end()) {
    return std::nullopt;
  }
  return found->second.party;
}

std::optional<Lobby::PartyId> Lobby::partyWithCode(std::string code) const {
  const auto found = this->_codes.find(normalizeCode(std::move(code)));
  if (found == this->_codes.end()) {
    return std::nullopt;
  }
  return found->second;
}

std::string Lobby::normalizeCode(std::string code) {
  for (char& character : code) {
    if (character >= 'a' && character <= 'z') {
      character = static_cast<char>(character - 'a' + 'A');
    }
  }
  return code;
}

void Lobby::applyChanges(Clock::time_point now) {
  for (const Change& change : this->_changes.drain()) {
    const auto party = this->_parties.find(change.party);
    if (party == this->_parties.end()) {
      continue;  // closed since
    }
    Client* const client = this->findClient(change.endpoint);
    if (change.change == SessionChange::Joined) {
      if (client == nullptr || client->party != change.party) {
        // Should never happen: the Lobby routes a client to a party before
        // it admits it there. The party drops it, so it plays in one party
        // at most.
        protocol::Packet quit;
        quit.payload =
            protocol::Disconnect{.reason = protocol::DisconnectReason::Quit};
        party->second.server->push({.from = change.endpoint, .packet = quit});
      }
      continue;
    }
    if (client == nullptr || !this->unbind(*client, change.party)) {
      continue;  // routed to another party since
    }
    if (change.change == SessionChange::Left) {
      this->_clients.erase(change.endpoint.toString());
      continue;
    }
    // Back in the lobby: the Lobby answers its packets from now on.
    client->lastSeen = now;
    client->lastRoomList.reset();
    this->_network.send(protocol::RoomLeft{}, client->endpoint);
  }
}

void Lobby::route(IncomingMessage message, Clock::time_point now) {
  Client* const client = this->findClient(message.from);
  if (client == nullptr) {
    this->connect(message, now);
    return;
  }
  if (!client->party) {
    this->handleVisitor(*client, message.packet.payload, now);
    return;
  }
  const PartyId partyId = *client->party;
  if (const auto* request =
          std::get_if<protocol::ConnectRequest>(&message.packet.payload)) {
    // The client restarted with the same address: take it out of its party
    // as if it sent ROOM_LEAVE. Once back in the lobby, its next
    // CONNECT_REQUEST is accepted.
    if (GameServer::isValidName(request->playerName)) {
      client->name = request->playerName;
    }
    message.packet.payload = protocol::RoomLeave{};
  }
  const auto party = this->_parties.find(partyId);
  if (party != this->_parties.end() &&
      !party->second.server->push(std::move(message))) {
    logLine(std::cerr, "[server] party ", partyId,
            " is overloaded, dropped a packet from ",
            client->endpoint.toString());
  }
}

void Lobby::connect(const IncomingMessage& message, Clock::time_point now) {
  const auto* request =
      std::get_if<protocol::ConnectRequest>(&message.packet.payload);
  if (request == nullptr) {
    return;  // only CONNECT_REQUEST is accepted from an unknown address
  }
  if (!GameServer::isValidName(request->playerName)) {
    this->_network.send(
        protocol::ConnectReject{.reason = protocol::RejectReason::InvalidName},
        message.from);
    return;
  }
  if (this->_clients.size() >= MAX_CLIENTS) {
    this->_network.send(
        protocol::ConnectReject{.reason = protocol::RejectReason::ServerFull},
        message.from);
    return;
  }
  this->_clients.emplace(message.from.toString(),
                         Client{
                             .endpoint = message.from,
                             .name = request->playerName,
                             .party = std::nullopt,
                             .lastSeen = now,
                             .lastRoomList = std::nullopt,
                         });
  logLine(std::cout, "[server] ", request->playerName,
          " joined the lobby from ", message.from.toString());
  this->_network.send(protocol::ConnectAccept{}, message.from);
}

void Lobby::handleVisitor(Client& client, const protocol::Payload& payload,
                          Clock::time_point now) {
  client.lastSeen = now;
  if (std::holds_alternative<protocol::RoomList>(payload)) {
    this->sendRoomList(client, now);
  } else if (const auto* create = std::get_if<protocol::RoomCreate>(&payload)) {
    this->create(client, create->visibility);
  } else if (const auto* join = std::get_if<protocol::RoomJoin>(&payload)) {
    this->join(client, *join);
  } else if (const auto* ping = std::get_if<protocol::Ping>(&payload)) {
    this->_network.send(protocol::Pong{.timestampMs = ping->timestampMs},
                        client.endpoint);
  } else if (std::holds_alternative<protocol::ConnectRequest>(payload)) {
    // Sent again by a client whose CONNECT_ACCEPT was lost.
    this->_network.send(protocol::ConnectAccept{}, client.endpoint);
  } else if (std::holds_alternative<protocol::RoomLeave>(payload)) {
    // Sent again by a client whose ROOM_LEFT was lost.
    this->_network.send(protocol::RoomLeft{}, client.endpoint);
  } else if (std::holds_alternative<protocol::Disconnect>(payload)) {
    logLine(std::cout, "[server] ", client.name, " left the lobby");
    this->_clients.erase(client.endpoint.toString());
  }
}

void Lobby::sendRoomList(Client& client, Clock::time_point now) {
  if (client.lastRoomList && now - *client.lastRoomList < ROOM_LIST_COOLDOWN) {
    return;
  }
  protocol::RoomListReply reply;
  for (const auto& [partyId, party] : this->_parties) {
    if (reply.rooms.size() == protocol::MAX_LISTED_ROOMS) {
      break;
    }
    if (party.code.empty()) {
      reply.rooms.push_back({
          .roomId = partyId,
          .players = static_cast<std::uint8_t>(party.players),
      });
    }
  }
  client.lastRoomList = now;
  this->_network.send(reply, client.endpoint);
}

void Lobby::create(Client& client, protocol::RoomVisibility visibility) {
  if (this->_parties.size() >= this->_maxParties) {
    this->_network.send(
        protocol::RoomReject{.reason = protocol::RejectReason::ServerFull},
        client.endpoint);
    return;
  }
  const std::string code = visibility == protocol::RoomVisibility::Private
                               ? this->generateCode()
                               : std::string{};
  this->admit(client, this->open(code));
}

void Lobby::join(Client& client, const protocol::RoomJoin& request) {
  std::variant<PartyId, protocol::RejectReason> placed;
  if (!request.code.empty()) {
    if (!protocol::isValidPartyCode(request.code)) {
      placed = protocol::RejectReason::InvalidPartyCode;
    } else {
      placed = this->place(normalizeCode(request.code));
    }
  } else if (request.roomId != 0) {
    placed = this->findPublic(request.roomId);
  } else {
    placed = this->place({});
  }
  if (const auto* reason = std::get_if<protocol::RejectReason>(&placed)) {
    this->_network.send(protocol::RoomReject{.reason = *reason},
                        client.endpoint);
    return;
  }
  this->admit(client, std::get<PartyId>(placed));
}

void Lobby::admit(Client& client, PartyId party) {
  Party& target = this->_parties.at(party);
  client.party = party;
  ++target.players;
  ++this->_players;
  if (!target.server->admit(client.endpoint, client.name)) {
    logLine(std::cerr, "[server] party ", party, " is overloaded, ",
            client.name, " stays in the lobby");
    this->unbind(client, party);
  }
}

std::variant<Lobby::PartyId, protocol::RejectReason> Lobby::place(
    const std::string& code) {
  if (code.empty()) {
    for (const auto& [partyId, party] : this->_parties) {
      if (party.code.empty() && party.players < protocol::MAX_PLAYERS) {
        return partyId;
      }
    }
  } else if (const auto found = this->_codes.find(code);
             found != this->_codes.end()) {
    if (this->_parties.at(found->second).players >= protocol::MAX_PLAYERS) {
      return protocol::RejectReason::PartyFull;
    }
    return found->second;
  }
  if (this->_parties.size() >= this->_maxParties) {
    return protocol::RejectReason::ServerFull;
  }
  return this->open(code);
}

std::variant<Lobby::PartyId, protocol::RejectReason> Lobby::findPublic(
    PartyId roomId) const {
  const auto found = this->_parties.find(roomId);
  if (found == this->_parties.end() || !found->second.code.empty()) {
    return protocol::RejectReason::RoomNotFound;
  }
  if (found->second.players >= protocol::MAX_PLAYERS) {
    return protocol::RejectReason::PartyFull;
  }
  return roomId;
}

Lobby::PartyId Lobby::open(const std::string& code) {
  const PartyId partyId = this->_nextPartyId++;
  auto server = std::make_unique<GameServer>(
      this->_network, std::random_device{}(),
      PartyLink{
          .id = partyId,
          .code = code,
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
          .code = code,
          .players = 0,
          .thread = std::jthread(
              [&game](const std::stop_token& token) { game.run(token); }),
      });
  if (code.empty()) {
    logLine(std::cout, "[server] party ", partyId, " opened");
  } else {
    this->_codes.emplace(code, partyId);
    logLine(std::cout, "[server] party ", partyId, " opened, code ", code);
  }
  return partyId;
}

std::string Lobby::generateCode() {
  std::uniform_int_distribution<int> digit(0, 9);
  std::string code(GENERATED_CODE_LENGTH, '0');
  do {
    for (char& character : code) {
      character = static_cast<char>('0' + digit(this->_random));
    }
  } while (this->_codes.contains(code));
  return code;
}

void Lobby::dropSilentVisitors(Clock::time_point now) {
  std::erase_if(this->_clients, [now](const auto& entry) {
    const Client& client = entry.second;
    if (client.party || now - client.lastSeen <= CLIENT_TIMEOUT) {
      return false;
    }
    logLine(std::cout, "[server] ", client.name, " left the lobby: timeout");
    return true;
  });
}

void Lobby::closeEmptyParties() {
  for (auto party = this->_parties.begin(); party != this->_parties.end();) {
    if (party->second.players != 0) {
      ++party;
      continue;
    }
    logLine(std::cout, "[server] party ", party->first, " closed");
    if (!party->second.code.empty()) {
      this->_codes.erase(party->second.code);
    }
    party = this->_parties.erase(party);
  }
}

void Lobby::sayGoodbyeToVisitors() {
  for (const auto& [key, client] : this->_clients) {
    if (client.party) {
      continue;  // told by its party
    }
    for (std::uint32_t attempt = 0; attempt < DISCONNECT_REPEAT; ++attempt) {
      this->_network.send(
          protocol::Disconnect{
              .reason = protocol::DisconnectReason::ServerShutdown,
          },
          client.endpoint);
    }
  }
}

bool Lobby::unbind(Client& client, PartyId party) {
  if (client.party != party) {
    return false;
  }
  client.party.reset();
  --this->_players;
  --this->_parties.at(party).players;
  return true;
}

Lobby::Client* Lobby::findClient(const Endpoint& endpoint) {
  const auto found = this->_clients.find(endpoint.toString());
  return found == this->_clients.end() ? nullptr : &found->second;
}

}  // namespace rtype::server
