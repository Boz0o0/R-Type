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

  if (this->_state == State::Connected &&
      now - this->_lastReceived > SERVER_TIMEOUT) {
    this->_state = State::Disconnected;
    std::cerr << "[client] lost the server " << this->_server.toString()
              << '\n';
    return;
  }
  if (this->_state == State::Connecting) {
    if (this->_lastSend && now - *this->_lastSend < CONNECT_RETRY_DELAY) {
      return;
    }
    if (this->_connectAttempts == MAX_CONNECT_ATTEMPTS) {
      this->_state = State::Unreachable;
      std::cerr << "[client] no answer from " << this->_server.toString()
                << '\n';
      return;
    }
    ++this->_connectAttempts;
    this->_lastSend = now;
    this->send(protocol::ConnectRequest{.playerName = this->_playerName});
  } else if (this->_state == State::Connected &&
             (!this->_lastSend || now - *this->_lastSend >= PING_INTERVAL)) {
    this->_lastSend = now;
    const auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch());
    const auto timestampMs = static_cast<std::uint32_t>(milliseconds.count());
    this->send(protocol::Ping{.timestampMs = timestampMs});
  }
}

void ServerConnection::disconnect() {
  if (this->_state != State::Connected) {
    return;
  }
  for (std::uint32_t attempt = 0; attempt < DISCONNECT_REPEAT; ++attempt) {
    this->send(
        protocol::Disconnect{.reason = protocol::DisconnectReason::Quit});
  }
  this->_state = State::Disconnected;
}

void ServerConnection::sendInput(std::uint8_t inputMask) {
  if (this->_state != State::Connected) {
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

std::optional<std::uint8_t> ServerConnection::playerId() const noexcept {
  return this->_playerId;
}

std::optional<protocol::RejectReason> ServerConnection::rejectReason()
    const noexcept {
  return this->_rejectReason;
}

std::uint32_t ServerConnection::lastServerTick() const noexcept {
  return this->_lastServerTick;
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
  if (const auto* accept = std::get_if<protocol::ConnectAccept>(&payload)) {
    if (this->_state == State::Connecting) {
      this->_state = State::Connected;
      this->_playerId = accept->playerId;
      std::cout << "[client] connected to " << this->_server.toString()
                << " as player " << static_cast<int>(accept->playerId) << '\n';
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
    if (this->_state == State::Connected) {
      this->_state = State::Disconnected;
      std::cout << "[client] disconnected by the server\n";
    }
  } else if (const auto* ping = std::get_if<protocol::Ping>(&payload)) {
    this->send(protocol::Pong{.timestampMs = ping->timestampMs});
  } else if (packet.header.type >= protocol::PacketType::GameStart) {
    if (const auto* snapshot = std::get_if<protocol::WorldSnapshot>(&payload)) {
      this->_lastServerTick =
          std::max(this->_lastServerTick, snapshot->serverTick);
    }
    this->_gamePackets.push_back(payload);
  }
}

void ServerConnection::send(const protocol::Payload& payload) {
  this->_socket.send(protocol::encode(payload, this->_sequence++),
                     this->_server);
}

}  // namespace rtype::client
