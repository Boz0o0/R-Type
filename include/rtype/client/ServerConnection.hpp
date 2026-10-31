#pragma once

#include <ampersand/net/UdpSocket.hpp>
#include <chrono>
#include <cstdint>
#include <optional>
#include <rtype/common/protocol/PacketCodec.hpp>
#include <rtype/common/protocol/Packets.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace rtype::client {

/** @brief What the player asks to play in: a new room, or one to join. */
using RoomRequest = std::variant<protocol::RoomCreate, protocol::RoomJoin>;

/**
 * @brief The link between the client and the server.
 *
 * Enters the lobby by sending CONNECT_REQUEST until the server answers, then
 * keeps the connection alive with PINGs. In the lobby it asks for the list
 * of the public rooms every ROOM_LIST_INTERVAL; enterRoom() and leaveRoom()
 * send their request until the server answers it. update() never waits for
 * the network, so it is called once per frame. The game packets of the room
 * are handed out by takePackets().
 */
class ServerConnection {
 public:
  using Clock = std::chrono::steady_clock;

  enum class State : std::uint8_t {
    /** @brief Sending CONNECT_REQUEST, no answer yet. */
    Connecting,
    /** @brief In the lobby: rooms() is refreshed, enterRoom() can be used. */
    InLobby,
    /** @brief Sending ROOM_CREATE or ROOM_JOIN, no answer yet. */
    Joining,
    /** @brief ROOM_JOINED received: room() and playerId() are set. */
    InRoom,
    /** @brief Sending ROOM_LEAVE, no answer yet. */
    Leaving,
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
  /** @brief Delay between two room requests without answer. */
  static constexpr std::chrono::milliseconds REQUEST_RETRY_DELAY{500};
  /** @brief Delay between two ROOM_LIST while in the lobby. */
  static constexpr std::chrono::milliseconds ROOM_LIST_INTERVAL{1000};
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
   * @brief Reads every packet waiting, then sends what is due: the
   *        CONNECT_REQUEST, the room request, a ROOM_LIST or a PING.
   *        Switches to Disconnected after SERVER_TIMEOUT of silence.
   *
   * @param now Current time.
   */
  void update(Clock::time_point now);

  /**
   * @brief Asks to open or join a room, if in the lobby. The request is sent
   *        by update() until the server answers.
   *
   * @param request ROOM_CREATE or ROOM_JOIN.
   */
  void enterRoom(const RoomRequest& request);

  /** @brief Asks to go back to the lobby, if in a room. */
  void leaveRoom();

  /** @brief Tells the server the player quits, if connected. */
  void disconnect();

  /** @brief Sends the keys held this frame (an InputFlag mask), if in a room.
   */
  void sendInput(std::uint8_t inputMask);

  /** @brief Game packets (0x20-0x3F) of the room since the last call. */
  [[nodiscard]] std::vector<protocol::Payload> takePackets();

  [[nodiscard]] State state() const noexcept;
  /** @brief Rejected, Unreachable or Disconnected: nothing more will come. */
  [[nodiscard]] bool hasEnded() const noexcept;
  /** @brief The room we play in, once in one. */
  [[nodiscard]] const std::optional<protocol::RoomJoined>& room()
      const noexcept;
  /** @brief Id given by the room, once in one. */
  [[nodiscard]] std::optional<std::uint8_t> playerId() const noexcept;
  /** @brief Why the server refused us, once rejected. */
  [[nodiscard]] std::optional<protocol::RejectReason> rejectReason()
      const noexcept;
  /** @brief Why the last room request was refused, until the next one. */
  [[nodiscard]] std::optional<protocol::RejectReason> roomRejectReason()
      const noexcept;
  /** @brief The public rooms of the last ROOM_LIST_REPLY. */
  [[nodiscard]] const std::vector<protocol::RoomInfo>& rooms() const noexcept;
  /** @brief serverTick of the newest WORLD_SNAPSHOT of the room. */
  [[nodiscard]] std::uint32_t lastServerTick() const noexcept;

 private:
  /** @brief CONNECT_ACCEPT received or not, the server talks to us. */
  [[nodiscard]] bool isConnected() const noexcept;
  void receivePackets(Clock::time_point now);
  void handlePacket(const protocol::Packet& packet);
  /** @brief Handles ROOM_LIST_REPLY, ROOM_JOINED, ROOM_REJECT and ROOM_LEFT.
   */
  void handleLobbyPacket(const protocol::Payload& payload);
  /** @brief Sends the CONNECT_REQUEST, or gives up. */
  void sendConnectRequest(Clock::time_point now);
  /** @brief Sends the pending room request, ROOM_LIST and PING when due. */
  void sendDue(Clock::time_point now);
  /** @brief Back in the lobby: forgets the room and its packets. */
  void enterLobby();
  void send(const protocol::Payload& payload);

  ampersand::net::UdpSocket _socket;
  ampersand::net::Endpoint _server;
  std::string _playerName;
  State _state{State::Connecting};
  std::optional<protocol::RoomJoined> _room;
  std::optional<protocol::RejectReason> _rejectReason;
  std::optional<protocol::RejectReason> _roomRejectReason;
  std::vector<protocol::RoomInfo> _rooms;
  /** @brief ROOM_CREATE, ROOM_JOIN or ROOM_LEAVE, sent until answered. */
  std::optional<protocol::Payload> _request;
  std::optional<Clock::time_point> _lastRequest;
  std::optional<Clock::time_point> _lastRoomList;
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
