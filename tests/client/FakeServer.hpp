#pragma once

#include <ampersand/net/UdpSocket.hpp>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <rtype/client/ServerConnection.hpp>
#include <rtype/common/protocol/PacketCodec.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <span>
#include <stdexcept>
#include <thread>

namespace rtype::client::test {

using namespace rtype::protocol;
using ampersand::net::Endpoint;
using ampersand::net::UdpSocket;
using State = ServerConnection::State;

inline const ServerConnection::Clock::time_point START =
    ServerConnection::Clock::now();

/** @brief A fake server: a raw socket that records who talks to it. */
struct FakeServer {
  UdpSocket socket;
  Endpoint client;

  /**
   * @brief Waits for the next valid packet and remembers its sender.
   * @throws std::runtime_error If nothing comes within a second.
   */
  Packet receive() {
    std::array<std::byte, MAX_DATAGRAM_SIZE + 1> buffer{};
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while (std::chrono::steady_clock::now() < deadline) {
      if (const auto length = this->socket.receive(buffer, this->client)) {
        Packet packet;
        if (decode(std::span(buffer).first(*length), packet) ==
            DecodeError::None) {
          return packet;
        }
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    throw std::runtime_error("no packet from the client");
  }

  void send(const Payload& payload, std::uint32_t sequence = 0) {
    this->socket.send(encode(payload, sequence), this->client);
  }
};

/** @brief Calls update() until the state changes or a second has passed. */
inline void updateUntilStateChanges(ServerConnection& connection,
                                    ServerConnection::Clock::time_point now) {
  const State before = connection.state();
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(1);
  while (connection.state() == before &&
         std::chrono::steady_clock::now() < deadline) {
    connection.update(now);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

/** @brief Connects the client, then reads its ACK of CONNECT_ACCEPT. */
inline void connectToServer(FakeServer& server, ServerConnection& connection) {
  connection.update(START);
  server.receive();
  server.send(ConnectAccept{.playerId = 0}, 1);
  updateUntilStateChanges(connection, START);
  server.receive();
}

}  // namespace rtype::client::test
