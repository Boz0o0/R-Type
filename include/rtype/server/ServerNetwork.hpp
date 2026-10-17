#pragma once

#include <ampersand/net/UdpSocket.hpp>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <rtype/common/protocol/PacketCodec.hpp>
#include <rtype/server/MessageQueue.hpp>
#include <rtype/server/RateLimiter.hpp>
#include <span>
#include <stop_token>
#include <thread>
#include <vector>

namespace rtype::server {

/** @brief A valid packet received by the server, and who sent it. */
struct IncomingMessage {
  ampersand::net::Endpoint from;
  protocol::Packet packet;
};

/** @brief An encoded packet, ready to be sent (and sent again). */
struct OutgoingDatagram {
  /** @brief Sequence written in its header. */
  std::uint32_t sequence{};
  /** @brief Its bytes, header included. */
  std::vector<std::byte> bytes;
};

/**
 * @brief The network thread of the server.
 *
 * As soon as it is built, a thread reads every datagram that arrives on the
 * socket, decodes it and pushes it into a MessageQueue. Datagrams that
 * protocol::decode() refuses (empty, too short, bad magic...) are logged and
 * dropped: they never reach the game and never crash the server. So are the
 * datagrams of a sender past MAX_PACKETS_PER_SECOND.
 *
 * The game thread takes the messages with receive() and answers with send().
 * Both threads share the socket, so every use of it locks a mutex.
 */
class ServerNetwork {
 public:
  /** @brief Most messages waiting between two game ticks; more are dropped. */
  static constexpr std::size_t MAX_QUEUED_MESSAGES = 4096;
  /**
   * @brief Datagrams accepted per second from one sender (IP + port).
   *
   * A client sends an input per frame (up to 144 with vsync on a fast
   * screen), an ACK per event and a PING per second: this leaves it room.
   */
  static constexpr std::uint32_t MAX_PACKETS_PER_SECOND = 500;
  /** @brief Senders counted per second; datagrams of more are dropped. */
  static constexpr std::size_t MAX_SENDERS_PER_SECOND = 1024;

  /**
   * @brief Binds the socket and starts the network thread.
   *
   * @param port UDP port to listen on, or 0 for a port picked by the system.
   * @throws ampersand::net::NetError If the port cannot be bound.
   */
  explicit ServerNetwork(std::uint16_t port);

  /** @brief Stops the network thread and waits for it. */
  ~ServerNetwork() = default;

  ServerNetwork(const ServerNetwork&) = delete;
  ServerNetwork& operator=(const ServerNetwork&) = delete;
  ServerNetwork(ServerNetwork&&) = delete;
  ServerNetwork& operator=(ServerNetwork&&) = delete;

  /** @brief The local port the server listens on. */
  [[nodiscard]] std::uint16_t localPort() const;

  /**
   * @brief Takes every packet received since the last call, oldest first.
   *
   * Never waits: returns an empty list when nothing has arrived.
   */
  [[nodiscard]] std::vector<IncomingMessage> receive();

  /**
   * @brief Encodes a payload and sends it. Can be called from any thread.
   *
   * A send that fails is logged and dropped, like a packet lost on the
   * network.
   *
   * @throws protocol::ProtocolError If the payload breaks the protocol.
   */
  void send(const protocol::Payload& payload,
            const ampersand::net::Endpoint& target);

  /**
   * @brief Encodes a payload with the next sequence of the server, without
   *        sending it, so that it can be sent again with the same bytes.
   *
   * @throws protocol::ProtocolError If the payload breaks the protocol.
   */
  [[nodiscard]] OutgoingDatagram encode(const protocol::Payload& payload);

  /**
   * @brief Sends bytes already encoded. Can be called from any thread.
   *
   * A send that fails is logged and dropped, like send().
   */
  void sendDatagram(std::span<const std::byte> datagram,
                    const ampersand::net::Endpoint& target);

 private:
  /** @brief Wait between two polls of the socket when nothing arrived. */
  static constexpr std::chrono::milliseconds IDLE_DELAY{1};

  /** @brief Body of the network thread. */
  void receiveLoop(const std::stop_token& stopToken);

  /**
   * @brief Reads, checks and queues one datagram.
   *
   * @param buffer Receive buffer, at least MAX_DATAGRAM_SIZE + 1 bytes.
   * @return false if no datagram was waiting.
   */
  bool receiveOne(std::span<std::byte> buffer);

  mutable std::mutex _socketMutex;
  ampersand::net::UdpSocket _socket;
  std::atomic<std::uint32_t> _sequence{0};
  MessageQueue<IncomingMessage> _queue{MAX_QUEUED_MESSAGES};
  /** @brief Only used by the network thread. */
  RateLimiter _rateLimiter{{
      .packetsPerSecond = MAX_PACKETS_PER_SECOND,
      .senders = MAX_SENDERS_PER_SECOND,
  }};
  // Last member: started once everything above is built, and stopped (then
  // joined) before anything above is destroyed.
  std::jthread _thread;
};

}  // namespace rtype::server
