#pragma once

#include <ampersand/net/UdpSocket.hpp>
#include <array>
#include <chrono>
#include <cstddef>
#include <optional>
#include <rtype/common/protocol/PacketCodec.hpp>
#include <rtype/server/GameServer.hpp>
#include <rtype/server/ServerNetwork.hpp>
#include <span>
#include <thread>
#include <variant>
#include <vector>

namespace rtype::server::test {

/** @brief A fake client: a raw socket speaking to a ServerNetwork. */
struct TestClient {
  ampersand::net::UdpSocket socket;
  ampersand::net::Endpoint server;

  explicit TestClient(const ServerNetwork& network)
      : server(ampersand::net::UdpSocket::resolve("127.0.0.1",
                                                  network.localPort())) {}

  void send(const protocol::Payload& payload, std::uint32_t sequence = 0) {
    this->socket.send(protocol::encode(payload, sequence), this->server);
  }

  /** @brief Next valid packet waiting, if any. */
  std::optional<protocol::Packet> poll() {
    std::array<std::byte, protocol::MAX_DATAGRAM_SIZE + 1> buffer{};
    ampersand::net::Endpoint from;
    while (const auto length = this->socket.receive(buffer, from)) {
      protocol::Packet packet;
      if (protocol::decode(std::span(buffer).first(*length), packet) ==
          protocol::DecodeError::None) {
        return packet;
      }
    }
    return std::nullopt;
  }
};

inline constexpr std::chrono::milliseconds DEFAULT_WAIT{1000};

/**
 * @brief Runs ticks of the game until the client receives a T, skipping the
 *        other packets (snapshots...).
 *
 * @param now Time given to every tick.
 * @return The payload, or std::nullopt after the wait.
 */
template <typename T>
std::optional<T> stepUntil(GameServer& game, TestClient& client,
                           GameServer::Clock::time_point now,
                           std::chrono::milliseconds wait = DEFAULT_WAIT) {
  const auto deadline = std::chrono::steady_clock::now() + wait;
  while (std::chrono::steady_clock::now() < deadline) {
    game.step(now);
    while (const auto packet = client.poll()) {
      if (const auto* payload = std::get_if<T>(&packet->payload)) {
        return *payload;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return std::nullopt;
}

/** @brief Waits until the network thread has queued at least one message. */
inline std::vector<IncomingMessage> receiveWithin(
    ServerNetwork& network, std::chrono::milliseconds wait = DEFAULT_WAIT) {
  const auto deadline = std::chrono::steady_clock::now() + wait;
  std::vector<IncomingMessage> messages;
  while (messages.empty() && std::chrono::steady_clock::now() < deadline) {
    messages = network.receive();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return messages;
}

}  // namespace rtype::server::test
