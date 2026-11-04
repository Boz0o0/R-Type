#include <algorithm>
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

namespace {

/** @brief The username and password of a REGISTER or a LOGIN. */
struct Credentials {
  const std::string& username;
  const std::string& password;
  bool creates;
};

std::optional<Credentials> credentialsOf(const protocol::Payload& payload) {
  if (const auto* request = std::get_if<protocol::Register>(&payload)) {
    return Credentials{
        .username = request->username,
        .password = request->password,
        .creates = true,
    };
  }
  if (const auto* request = std::get_if<protocol::Login>(&payload)) {
    return Credentials{
        .username = request->username,
        .password = request->password,
        .creates = false,
    };
  }
  return std::nullopt;
}

}  // namespace

Lobby::Lobby(ServerNetwork& network, std::size_t maxParties)
    : _network(network), _maxParties(maxParties) {}

Lobby::Lobby(ServerNetwork& network, AccountStore& accounts,
             std::size_t maxParties)
    : _network(network),
      _auth(std::make_unique<AuthWorker>(accounts)),
      _maxParties(maxParties) {}

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
  this->sayGoodbye();
  this->_clients.clear();
  this->_codes.clear();
}

void Lobby::step() { this->step(Clock::now()); }

void Lobby::step(Clock::time_point now) {
  this->applyChanges(now);
  this->applyLogins(now);
  // Before the requests: a party left empty no longer counts in the limit.
  this->closeEmptyParties();
  for (IncomingMessage& message : this->_network.receive()) {
    this->route(std::move(message), now);
  }
  this->dropSilentVisitors(now);
  this->closeEmptyParties();
}

std::size_t Lobby::partyCount() const noexcept { return this->_parties.size(); }

std::size_t Lobby::playerCount() const noexcept {
  return static_cast<std::size_t>(std::ranges::count_if(
      this->_clients,
      [](const auto& entry) { return entry.second.party.has_value(); }));
}

std::size_t Lobby::visitorCount() const noexcept {
  return static_cast<std::size_t>(std::ranges::count_if(
      this->_clients,
      [](const auto& entry) { return !entry.second.party.has_value(); }));
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
  if (this->isConnect(message.packet.payload)) {
    if (now - client->connectedAt <= LATE_CONNECT_WINDOW) {
      return;  // a late copy, sent before the client got CONNECT_ACCEPT
    }
    // The client restarted with the same address: take it out of its party
    // as if it sent ROOM_LEAVE. Once back in the lobby, its next
    // CONNECT_REQUEST, REGISTER or LOGIN is checked again.
    const auto* request =
        std::get_if<protocol::ConnectRequest>(&message.packet.payload);
    if (request != nullptr && GameServer::isValidName(request->playerName)) {
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
  const protocol::Payload& payload = message.packet.payload;
  if (!this->isConnect(payload)) {
    return;  // an unknown address may only ask to enter the lobby
  }
  if (const auto credentials = credentialsOf(payload)) {
    this->startLogin(message.from, credentials->username, credentials->password,
                     credentials->creates, now);
    return;
  }
  const auto& request = std::get<protocol::ConnectRequest>(payload);
  if (!GameServer::isValidName(request.playerName)) {
    this->refuse(message.from, protocol::RejectReason::InvalidName);
  } else if (this->_clients.size() >= MAX_CLIENTS) {
    this->refuse(message.from, protocol::RejectReason::ServerFull);
  } else {
    this->admit(message.from, request.playerName, std::nullopt, now);
  }
}

void Lobby::refuse(const Endpoint& endpoint, protocol::RejectReason reason) {
  this->_network.send(protocol::ConnectReject{.reason = reason}, endpoint);
}

void Lobby::admit(const Endpoint& endpoint, const std::string& name,
                  std::optional<AccountStore::AccountId> account,
                  Clock::time_point now) {
  this->_clients.emplace(endpoint.toString(), Client{
                                                  .endpoint = endpoint,
                                                  .name = name,
                                                  .party = std::nullopt,
                                                  .lastSeen = now,
                                                  .connectedAt = now,
                                                  .lastRoomList = std::nullopt,
                                                  .account = account,
                                              });
  logLine(std::cout, "[server] ", name, " joined the lobby from ",
          endpoint.toString());
  this->_network.send(protocol::ConnectAccept{}, endpoint);
}

std::optional<protocol::RejectReason> Lobby::checkCredentials(
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    const std::string& username, const std::string& password, bool creates) {
  if (!GameServer::isValidName(username)) {
    return creates ? protocol::RejectReason::InvalidName
                   : protocol::RejectReason::BadCredentials;
  }
  if (password.empty() || password.size() > protocol::MAX_PASSWORD_LENGTH) {
    return creates ? protocol::RejectReason::InvalidPassword
                   : protocol::RejectReason::BadCredentials;
  }
  return std::nullopt;
}

void Lobby::startLogin(const Endpoint& endpoint, const std::string& username,
                       const std::string& password, bool creates,
                       Clock::time_point now) {
  const std::string key = endpoint.toString();
  if (this->_pendingLogins.contains(key)) {
    return;  // a copy sent again while its password is checked
  }
  if (const auto reason = checkCredentials(username, password, creates)) {
    this->refuse(endpoint, *reason);
    return;
  }
  // Checked before the password: hashing it is the slow part.
  if (this->_clients.size() + this->_pendingLogins.size() >= MAX_CLIENTS ||
      this->_pendingLogins.size() >= AuthWorker::MAX_PENDING) {
    this->refuse(endpoint, protocol::RejectReason::ServerFull);
    return;
  }
  if (!this->_loginLimiter.tryAttempt(endpoint.address, now)) {
    logLine(std::cout, "[server] too many login attempts from ",
            endpoint.address);
    this->refuse(endpoint, protocol::RejectReason::TooManyAttempts);
    return;
  }
  if (!this->_auth->submit(AuthJob{
          .endpoint = endpoint,
          .username = username,
          .password = password,
          .creates = creates,
      })) {
    this->_loginLimiter.refund(endpoint.address);
    this->refuse(endpoint, protocol::RejectReason::ServerFull);
    return;
  }
  this->_pendingLogins.insert(key);
}

void Lobby::applyLogins(Clock::time_point now) {
  if (this->_auth == nullptr) {
    return;
  }
  for (AuthResult& result : this->_auth->results()) {
    this->_pendingLogins.erase(result.endpoint.toString());
    if (const auto* reason =
            std::get_if<protocol::RejectReason>(&result.outcome)) {
      this->refuse(result.endpoint, *reason);
      continue;
    }
    // The right password: only the failed attempts count.
    this->_loginLimiter.refund(result.endpoint.address);
    const auto account = std::get<AccountStore::AccountId>(result.outcome);
    if (this->findClient(result.endpoint) != nullptr) {
      continue;  // cannot happen: a pending address is not a client
    }
    if (this->isLoggedIn(account)) {
      this->refuse(result.endpoint, protocol::RejectReason::AlreadyLoggedIn);
    } else if (this->_clients.size() >= MAX_CLIENTS) {
      this->refuse(result.endpoint, protocol::RejectReason::ServerFull);
    } else {
      this->admit(result.endpoint, result.username, account, now);
    }
  }
}

bool Lobby::isConnect(const protocol::Payload& payload) const {
  if (this->_auth == nullptr) {
    return std::holds_alternative<protocol::ConnectRequest>(payload);
  }
  return credentialsOf(payload).has_value();
}

bool Lobby::isLoggedIn(AccountStore::AccountId account) const {
  return std::ranges::any_of(this->_clients, [account](const auto& entry) {
    return entry.second.account == account;
  });
}

void Lobby::handleVisitor(Client& client, const protocol::Payload& payload,
                          Clock::time_point now) {
  client.lastSeen = now;
  if (std::holds_alternative<protocol::RoomList>(payload)) {
    this->sendRoomList(client, now);
  } else if (const auto* create = std::get_if<protocol::RoomCreate>(&payload)) {
    this->create(client, *create);
  } else if (const auto* join = std::get_if<protocol::RoomJoin>(&payload)) {
    this->join(client, *join);
  } else if (const auto* ping = std::get_if<protocol::Ping>(&payload)) {
    this->_network.send(protocol::Pong{.timestampMs = ping->timestampMs},
                        client.endpoint);
  } else if (const auto* request =
                 std::get_if<protocol::ConnectRequest>(&payload);
             request != nullptr && this->_auth == nullptr) {
    // Sent again by a client whose CONNECT_ACCEPT was lost, or by a client
    // that restarted with the same address: check its name again.
    if (!GameServer::isValidName(request->playerName)) {
      this->_network.send(
          protocol::ConnectReject{
              .reason = protocol::RejectReason::InvalidName,
          },
          client.endpoint);
      logLine(std::cout, "[server] ", client.name,
              " left the lobby: invalid name");
      this->_clients.erase(client.endpoint.toString());
      return;
    }
    client.name = request->playerName;
    client.connectedAt = now;
    this->_network.send(protocol::ConnectAccept{}, client.endpoint);
  } else if (const auto credentials = credentialsOf(payload);
             credentials && this->_auth != nullptr) {
    if (credentials->username == client.name &&
        now - client.connectedAt <= LATE_CONNECT_WINDOW) {
      // Sent again by a client whose CONNECT_ACCEPT was lost.
      client.connectedAt = now;
      this->_network.send(protocol::ConnectAccept{}, client.endpoint);
      return;
    }
    // A client that restarted with the same address: it logs out, then its
    // new login is checked like the one of a new address.
    const Endpoint endpoint = client.endpoint;
    logLine(std::cout, "[server] ", client.name,
            " left the lobby: logs in again");
    this->_clients.erase(endpoint.toString());
    this->startLogin(endpoint, credentials->username, credentials->password,
                     credentials->creates, now);
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

void Lobby::create(Client& client, const protocol::RoomCreate& request) {
  Placement placed = protocol::RejectReason::ServerFull;
  if (this->_parties.size() < this->_maxParties) {
    placed = this->open(request.visibility == protocol::RoomVisibility::Private
                            ? this->generateCode()
                            : std::string{});
  }
  this->answer(client, request.requestId, placed);
}

void Lobby::join(Client& client, const protocol::RoomJoin& request) {
  Placement placed;
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
  this->answer(client, request.requestId, placed);
}

void Lobby::answer(Client& client, std::uint8_t requestId,
                   const Placement& placed) {
  if (const auto* reason = std::get_if<protocol::RejectReason>(&placed)) {
    this->_network.send(
        protocol::RoomReject{.requestId = requestId, .reason = *reason},
        client.endpoint);
    return;
  }
  const PartyId party = std::get<PartyId>(placed);
  Party& target = this->_parties.at(party);
  client.party = party;
  ++target.players;
  if (!target.server->admit(client.endpoint, client.name, requestId)) {
    logLine(std::cerr, "[server] party ", party, " is overloaded, ",
            client.name, " stays in the lobby");
    this->unbind(client, party);
  }
}

Lobby::Placement Lobby::place(const std::string& code) {
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

Lobby::Placement Lobby::findPublic(PartyId roomId) const {
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
    if (client.party || now - client.lastSeen <= GameServer::CLIENT_TIMEOUT) {
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

void Lobby::sayGoodbye() {
  // A party tells Left for every client it said goodbye to (and for those
  // that left before the end). Every other client is still waiting.
  for (const Change& change : this->_changes.drain()) {
    if (change.change == SessionChange::Left) {
      this->_clients.erase(change.endpoint.toString());
    }
  }
  for (const auto& [key, client] : this->_clients) {
    GameServer::sendDisconnect(this->_network, client.endpoint,
                               protocol::DisconnectReason::ServerShutdown);
  }
}

bool Lobby::unbind(Client& client, PartyId party) {
  if (client.party != party) {
    return false;
  }
  client.party.reset();
  --this->_parties.at(party).players;
  return true;
}

Lobby::Client* Lobby::findClient(const Endpoint& endpoint) {
  const auto found = this->_clients.find(endpoint.toString());
  return found == this->_clients.end() ? nullptr : &found->second;
}

}  // namespace rtype::server
