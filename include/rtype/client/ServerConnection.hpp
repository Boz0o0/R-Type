#pragma once

#include <ampersand/net/UdpSocket.hpp>
#include <chrono>
#include <cstdint>
#include <optional>
#include <rtype/common/protocol/PacketCodec.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace rtype::client {

/**
 * @brief The link between the client and the server.
 *
 * Joins the game by sending CONNECT_REQUEST until the server answers, then
 * keeps the connection alive with PINGs. update() never waits for the
 * network, so it is called once per frame. The game packets it receives are
 * handed out by takePackets().
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
    /** @brief Closed by either side, or the server stopped answering. */
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
  /** @brief Silence from the server after which it is considered gone. */
  static constexpr std::chrono::milliseconds SERVER_TIMEOUT{
      protocol::TIMEOUT_MS};
  /** @brief How long a reliable sequence is kept to drop its resent copies. */
  static constexpr std::chrono::milliseconds DUPLICATE_WINDOW{
      protocol::TIMEOUT_MS};

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
   *        it is time to. Switches to Disconnected after SERVER_TIMEOUT of
   *        silence.
   *
   * @param now Current time.
   */
  void update(Clock::time_point now);

  /** @brief Tells the server the player quits, if connected. */
  void disconnect();

  /** @brief Sends the keys held this frame (an InputFlag mask), if connected.
   */
  void sendInput(std::uint8_t inputMask);

  /** @brief Game packets (0x20 and up) received since the last call. */
  [[nodiscard]] std::vector<protocol::Payload> takePackets();

  [[nodiscard]] State state() const noexcept;
  /** @brief Id given by the server, once connected. */
  [[nodiscard]] std::optional<std::uint8_t> playerId() const noexcept;
  /** @brief Why the server refused us, once rejected. */
  [[nodiscard]] std::optional<protocol::RejectReason> rejectReason()
      const noexcept;
  /** @brief serverTick of the newest WORLD_SNAPSHOT received. */
  [[nodiscard]] std::uint32_t lastServerTick() const noexcept;

 private:
  void receivePackets(Clock::time_point now);
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
  std::uint32_t _clientTick{0};
  Clock::time_point _lastReceived;
  std::unordered_map<std::uint32_t, Clock::time_point> _handledSequences;
  std::vector<protocol::Payload> _gamePackets;
};

}  // namespace rtype::client
