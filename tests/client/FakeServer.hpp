#pragma once

#include <gtest/gtest.h>

#include <ampersand/net/UdpSocket.hpp>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <rtype/client/ServerConnection.hpp>
#include <rtype/common/protocol/PacketCodec.hpp>
#include <rtype/common/protocol/Packets.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <span>
#include <stdexcept>
#include <string>
#include <thread>
#include <variant>

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

  /**
   * @brief Waits for the next T, skipping the other packets (PING,
   *        ROOM_LIST...).
   * @throws std::runtime_error If no T comes within a second.
   */
  template <typename T>
  T receive() {
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while (std::chrono::steady_clock::now() < deadline) {
      const Packet packet = this->receive();
      if (const auto* payload = std::get_if<T>(&packet.payload)) {
        return *payload;
      }
    }
    throw std::runtime_error("not the packet expected from the client");
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

/** @brief Brings the client into the lobby. */
/** @brief Credentials given to logIn() by the tests. */
inline const std::string USERNAME = "Bob";
inline const std::string PASSWORD = "hunter2";

/** @brief Starts connecting with LOGIN, if not done yet. */
inline void logIn(ServerConnection& connection) {
  connection.logIn(USERNAME, PASSWORD, false);
}

inline void connectToLobby(FakeServer& server, ServerConnection& connection) {
  logIn(connection);
  connection.update(START);
  server.receive<Login>();
  server.send(ConnectAccept{});
  updateUntilStateChanges(connection, START);
  ASSERT_EQ(connection.state(), State::InLobby);
}

/** @brief Header sequence of the ROOM_JOINED sent by enterRoom(). */
inline constexpr std::uint32_t JOINED_SEQUENCE = 100;

/**
 * @brief Brings the client into room 3 (ROOM_JOINED with JOINED_SEQUENCE),
 *        then reads its ACK.
 */
inline void enterRoom(FakeServer& server, ServerConnection& connection,
                      std::uint8_t playerId = 0) {
  connectToLobby(server, connection);
  connection.enterRoom(RoomJoin{});
  connection.update(START);
  const auto request = server.receive<RoomJoin>();
  server.send(
      RoomJoined{
          .requestId = request.requestId, .roomId = 3, .playerId = playerId},
      JOINED_SEQUENCE);
  updateUntilStateChanges(connection, START);
  ASSERT_EQ(connection.state(), State::InRoom);
  EXPECT_EQ(server.receive<Ack>().sequence, JOINED_SEQUENCE);
}

}  // namespace rtype::client::test
