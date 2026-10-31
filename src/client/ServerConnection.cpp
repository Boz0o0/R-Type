#include <algorithm>
#include <ampersand/net/UdpSocket.hpp>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <rtype/client/ServerConnection.hpp>
#include <rtype/common/protocol/PacketCodec.hpp>
#include <rtype/common/protocol/Packets.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace rtype::client {

using ampersand::net::Endpoint;
using ampersand::net::NetError;
using ampersand::net::UdpSocket;

ServerConnection::ServerConnection(const std::string& host, std::uint16_t port,
                                   std::string playerName)
    : _server(UdpSocket::resolve(host, port)),
      _playerName(std::move(playerName)) {}

void ServerConnection::update(Clock::time_point now) {
  this->receivePackets(now);
  std::erase_if(this->_handledSequences, [now](const auto& entry) {
    return now - entry.second > DUPLICATE_WINDOW;
  });

  if (this->isConnected() && now - this->_lastReceived > SERVER_TIMEOUT) {
    this->_state = State::Disconnected;
    std::cerr << "[client] lost the server " << this->_server.toString()
              << '\n';
    return;
  }
  if (this->_state == State::Connecting) {
    this->sendConnectRequest(now);
  } else if (this->isConnected()) {
    this->sendDue(now);
  }
}

void ServerConnection::enterRoom(const RoomRequest& request) {
  if (this->_state != State::InLobby) {
    return;
  }
  this->_request = std::visit(
      [](const auto& value) { return protocol::Payload{value}; }, request);
  this->_lastRequest.reset();
  this->_roomRejectReason.reset();
  this->_state = State::Joining;
}

void ServerConnection::leaveRoom() {
  if (this->_state != State::InRoom) {
    return;
  }
  this->_request = protocol::RoomLeave{};
  this->_lastRequest.reset();
  this->_gamePackets.clear();
  this->_state = State::Leaving;
}

void ServerConnection::disconnect() {
  if (!this->isConnected()) {
    return;
  }
  for (std::uint32_t attempt = 0; attempt < DISCONNECT_REPEAT; ++attempt) {
    this->send(
        protocol::Disconnect{.reason = protocol::DisconnectReason::Quit});
  }
  this->_state = State::Disconnected;
}

void ServerConnection::sendInput(std::uint8_t inputMask) {
  if (this->_state != State::InRoom) {
    return;
  }
  this->send(protocol::PlayerInput{
      .clientTick = this->_clientTick++,
      .inputMask = inputMask,
  });
}

std::vector<protocol::Payload> ServerConnection::takePackets() {
  return std::exchange(this->_gamePackets, {});
}

ServerConnection::State ServerConnection::state() const noexcept {
  return this->_state;
}

bool ServerConnection::hasEnded() const noexcept {
  return this->_state == State::Rejected ||
         this->_state == State::Unreachable ||
         this->_state == State::Disconnected;
}

const std::optional<protocol::RoomJoined>& ServerConnection::room()
    const noexcept {
  return this->_room;
}

std::optional<std::uint8_t> ServerConnection::playerId() const noexcept {
  if (!this->_room) {
    return std::nullopt;
  }
  return this->_room->playerId;
}

std::optional<protocol::RejectReason> ServerConnection::rejectReason()
    const noexcept {
  return this->_rejectReason;
}

std::optional<protocol::RejectReason> ServerConnection::roomRejectReason()
    const noexcept {
  return this->_roomRejectReason;
}

const std::vector<protocol::RoomInfo>& ServerConnection::rooms()
    const noexcept {
  return this->_rooms;
}

std::uint32_t ServerConnection::lastServerTick() const noexcept {
  return this->_lastServerTick;
}

bool ServerConnection::isConnected() const noexcept {
  return this->_state == State::InLobby || this->_state == State::Joining ||
         this->_state == State::InRoom || this->_state == State::Leaving;
}

void ServerConnection::receivePackets(Clock::time_point now) {
  // +1 so that a datagram too large is reported as such instead of being cut.
  std::array<std::byte, protocol::MAX_DATAGRAM_SIZE + 1> buffer{};
  Endpoint from;
  try {
    while (const auto length = this->_socket.receive(buffer, from)) {
      protocol::Packet packet;
      if (from != this->_server ||
          protocol::decode(std::span(buffer).first(*length), packet) !=
              protocol::DecodeError::None) {
        continue;  // not from our server, or invalid: drop it
      }
      this->_lastReceived = now;
      if (packet.header.isReliable()) {
        this->send(protocol::Ack{.sequence = packet.header.sequence});
        if (!this->_handledSequences.try_emplace(packet.header.sequence, now)
                 .second) {
          continue;
        }
      }
      this->handlePacket(packet);
    }
  } catch (const NetError& error) {
    // Windows reports "port unreachable" while the server is not started
    // yet: keep trying until MAX_CONNECT_ATTEMPTS.
    std::cerr << "[client] " << error.what() << '\n';
  }
}

void ServerConnection::handlePacket(const protocol::Packet& packet) {
  const protocol::Payload& payload = packet.payload;
  if (std::holds_alternative<protocol::ConnectAccept>(payload)) {
    if (this->_state == State::Connecting) {
      std::cout << "[client] connected to " << this->_server.toString() << '\n';
      this->enterLobby();
    }
  } else if (const auto* reject =
                 std::get_if<protocol::ConnectReject>(&payload)) {
    if (this->_state == State::Connecting) {
      this->_state = State::Rejected;
      this->_rejectReason = reject->reason;
      std::cerr << "[client] the server refused the connection (reason "
                << static_cast<int>(reject->reason) << ")\n";
    }
  } else if (std::holds_alternative<protocol::Disconnect>(payload)) {
    if (this->isConnected()) {
      this->_state = State::Disconnected;
      std::cout << "[client] disconnected by the server\n";
    }
  } else if (const auto* ping = std::get_if<protocol::Ping>(&payload)) {
    this->send(protocol::Pong{.timestampMs = ping->timestampMs});
  } else if (protocol::isGamePacket(packet.header.type)) {
    if (this->_state != State::Joining && this->_state != State::InRoom) {
      return;  // from a room we left
    }
    if (const auto* snapshot = std::get_if<protocol::WorldSnapshot>(&payload)) {
      this->_lastServerTick =
          std::max(this->_lastServerTick, snapshot->serverTick);
    }
    this->_gamePackets.push_back(payload);
  } else {
    this->handleLobbyPacket(payload);
  }
}

void ServerConnection::handleLobbyPacket(const protocol::Payload& payload) {
  if (const auto* list = std::get_if<protocol::RoomListReply>(&payload)) {
    this->_rooms = list->rooms;
  } else if (const auto* joined = std::get_if<protocol::RoomJoined>(&payload)) {
    if (this->_state == State::Joining) {
      this->_state = State::InRoom;
      this->_room = *joined;
      this->_request.reset();
      std::cout << "[client] joined room " << joined->roomId << " as player "
                << static_cast<int>(joined->playerId) << '\n';
    } else if (this->_state == State::InLobby) {
      // A late answer put us in a room we no longer want: leave it.
      this->_room = *joined;
      this->_state = State::InRoom;
      this->leaveRoom();
    }
  } else if (const auto* refused =
                 std::get_if<protocol::RoomReject>(&payload)) {
    if (this->_state == State::Joining) {
      this->enterLobby();
      this->_roomRejectReason = refused->reason;
    }
  } else if (std::holds_alternative<protocol::RoomLeft>(payload) &&
             this->_state == State::Leaving) {
    this->enterLobby();
  }
}

void ServerConnection::sendConnectRequest(Clock::time_point now) {
  if (this->_lastSend && now - *this->_lastSend < CONNECT_RETRY_DELAY) {
    return;
  }
  if (this->_connectAttempts == MAX_CONNECT_ATTEMPTS) {
    this->_state = State::Unreachable;
    std::cerr << "[client] no answer from " << this->_server.toString() << '\n';
    return;
  }
  ++this->_connectAttempts;
  this->_lastSend = now;
  this->send(protocol::ConnectRequest{.playerName = this->_playerName});
}

void ServerConnection::sendDue(Clock::time_point now) {
  if (this->_request && (!this->_lastRequest ||
                         now - *this->_lastRequest >= REQUEST_RETRY_DELAY)) {
    this->_lastRequest = now;
    this->send(*this->_request);
  }
  if (this->_state == State::InLobby &&
      (!this->_lastRoomList ||
       now - *this->_lastRoomList >= ROOM_LIST_INTERVAL)) {
    this->_lastRoomList = now;
    this->send(protocol::RoomList{});
  }
  if (!this->_lastSend || now - *this->_lastSend >= PING_INTERVAL) {
    this->_lastSend = now;
    const auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch());
    const auto timestampMs = static_cast<std::uint32_t>(milliseconds.count());
    this->send(protocol::Ping{.timestampMs = timestampMs});
  }
}

void ServerConnection::enterLobby() {
  this->_state = State::InLobby;
  this->_room.reset();
  this->_request.reset();
  this->_lastRoomList.reset();
  this->_gamePackets.clear();
  this->_lastServerTick = 0;
}

void ServerConnection::send(const protocol::Payload& payload) {
  this->_socket.send(protocol::encode(payload, this->_sequence++),
                     this->_server);
}

}  // namespace rtype::client
