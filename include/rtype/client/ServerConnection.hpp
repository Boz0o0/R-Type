#pragma once

#include <ampersand/net/UdpSocket.hpp>
#include <chrono>
#include <cstdint>
#include <optional>
#include <rtype/common/protocol/PacketCodec.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <string>

namespace rtype::client {

/**
 * @brief The link between the client and the server.
 *
 * Joins the game by sending CONNECT_REQUEST until the server answers, then
 * keeps the connection alive with PINGs. update() never waits for the
 * network, so it is called once per frame.
 */
class ServerConnection {
 public:
  using Clock = std::chrono::steady_clock;

  enum class State : std::uint8_t {
    /** @brief Sending CONNECT_REQUEST, no answer yet. */
    Connecting,
    /** @brief CONNECT_ACCEPT received: playerId() is set. */
    Connected,
    /** @brief The server sent CONNECT_REJECT: rejectReason() is set. */
    Rejected,
    /** @brief No answer after MAX_CONNECT_ATTEMPTS requests. */
    Unreachable,
    /** @brief The server or the client closed the connection. */
    Disconnected,
  };

  /** @brief Delay between two CONNECT_REQUEST without answer. */
  static constexpr std::chrono::milliseconds CONNECT_RETRY_DELAY{500};
  /** @brief CONNECT_REQUEST sent before giving up. */
  static constexpr std::uint32_t MAX_CONNECT_ATTEMPTS = 10;
  /** @brief Delay between two PINGs while connected. */
  static constexpr std::chrono::milliseconds PING_INTERVAL{
      protocol::PING_INTERVAL_MS};
  /** @brief DISCONNECT is not reliable, so it is sent this many times. */
  static constexpr std::uint32_t DISCONNECT_REPEAT = 3;

  /**
   * @brief Opens a socket on a port picked by the system. Nothing is sent
   *        before the first update().
   *
   * @param host Host name or address of the server.
   * @param port UDP port of the server.
   * @param playerName Name sent in CONNECT_REQUEST (1 to 16 bytes).
   * @throws ampersand::net::NetError If the host cannot be resolved.
   */
  ServerConnection(const std::string& host, std::uint16_t port,
                   std::string playerName);

  /**
   * @brief Reads every packet waiting, then sends CONNECT_REQUEST or PING if
   *        it is time to.
   *
   * @param now Current time.
   */
  void update(Clock::time_point now);

  /** @brief Tells the server the player quits, if connected. */
  void disconnect();

  [[nodiscard]] State state() const noexcept;
  /** @brief Id given by the server, once connected. */
  [[nodiscard]] std::optional<std::uint8_t> playerId() const noexcept;
  /** @brief Why the server refused us, once rejected. */
  [[nodiscard]] std::optional<protocol::RejectReason> rejectReason()
      const noexcept;
  /** @brief serverTick of the newest WORLD_SNAPSHOT received. */
  [[nodiscard]] std::uint32_t lastServerTick() const noexcept;

 private:
  void receivePackets();
  void handlePacket(const protocol::Packet& packet);
  void send(const protocol::Payload& payload);

  ampersand::net::UdpSocket _socket;
  ampersand::net::Endpoint _server;
  std::string _playerName;
  State _state{State::Connecting};
  std::optional<std::uint8_t> _playerId;
  std::optional<protocol::RejectReason> _rejectReason;
  std::uint32_t _lastServerTick{0};
  std::uint32_t _connectAttempts{0};
  std::optional<Clock::time_point> _lastSend;
  std::uint32_t _sequence{0};
};

}  // namespace rtype::client
