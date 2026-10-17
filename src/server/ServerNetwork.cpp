#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <optional>
#include <rtype/common/protocol/PacketCodec.hpp>
#include <rtype/common/protocol/Packets.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <rtype/server/ServerNetwork.hpp>
#include <span>
#include <stop_token>
#include <thread>
#include <utility>
#include <vector>

namespace rtype::server {

using ampersand::net::Endpoint;
using ampersand::net::NetError;

ServerNetwork::ServerNetwork(std::uint16_t port)
    : _socket(port), _thread([this](const std::stop_token& stopToken) {
        this->receiveLoop(stopToken);
      }) {}

std::uint16_t ServerNetwork::localPort() const {
  const std::scoped_lock lock(this->_socketMutex);
  return this->_socket.localPort();
}

std::vector<IncomingMessage> ServerNetwork::receive() {
  return this->_queue.drain();
}

void ServerNetwork::send(const protocol::Payload& payload,
                         const Endpoint& target) {
  const auto datagram = protocol::encode(payload, this->_sequence++);
  try {
    const std::scoped_lock lock(this->_socketMutex);
    this->_socket.send(datagram, target);
  } catch (const NetError& error) {
    std::cerr << "[server] " << error.what() << '\n';
  }
}

void ServerNetwork::receiveLoop(const std::stop_token& stopToken) {
  // +1 so that a datagram too large is reported as such instead of being cut.
  std::array<std::byte, protocol::MAX_DATAGRAM_SIZE + 1> buffer{};
  while (!stopToken.stop_requested()) {
    if (!this->receiveOne(buffer)) {
      std::this_thread::sleep_for(IDLE_DELAY);
    }
  }
}

bool ServerNetwork::receiveOne(std::span<std::byte> buffer) {
  Endpoint from;
  std::optional<std::size_t> length;
  try {
    const std::scoped_lock lock(this->_socketMutex);
    length = this->_socket.receive(buffer, from);
  } catch (const NetError& error) {
    // Windows reports an ICMP "port unreachable" from a closed client as a
    // receive error: it must not stop the thread.
    std::cerr << "[server] " << error.what() << '\n';
    return false;
  }
  if (!length) {
    return false;
  }

  protocol::Packet packet;
  const protocol::DecodeError error =
      protocol::decode(buffer.first(*length), packet);
  if (error == protocol::DecodeError::None) {
    if (!this->_queue.push({.from = from, .packet = std::move(packet)})) {
      std::cerr << "[server] queue full, dropped a packet from "
                << from.toString() << '\n';
    }
    return true;
  }
  if (error == protocol::DecodeError::VersionMismatch &&
      packet.header.type == protocol::PacketType::ConnectRequest) {
    this->send(
        protocol::ConnectReject{
            .reason = protocol::RejectReason::VersionMismatch,
        },
        from);
  }
  std::cerr << "[server] dropped a packet from " << from.toString() << ": "
            << protocol::toString(error) << '\n';
  return true;
}

}  // namespace rtype::server
