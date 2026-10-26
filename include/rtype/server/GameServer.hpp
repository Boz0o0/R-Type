#pragma once

#include <ampersand/net/UdpSocket.hpp>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <random>
#include <rtype/common/protocol/PacketCodec.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <rtype/server/Game.hpp>
#include <rtype/server/GameEvents.hpp>
#include <rtype/server/MessageQueue.hpp>
#include <rtype/server/ReliableOutbox.hpp>
#include <rtype/server/ServerNetwork.hpp>
#include <stop_token>
#include <string>
#include <string_view>
#include <vector>

namespace rtype::server {

/** @brief A client that joined the game. */
struct ClientSession {
  std::uint8_t playerId{};
  ampersand::net::Endpoint endpoint;
  std::string name;
  /** @brief When the last valid packet of this client arrived. */
  std::chrono::steady_clock::time_point lastSeen;
  /** @brief clientTick of the newest PLAYER_INPUT used. */
  std::optional<std::uint32_t> lastInputTick;
  /** @brief Reliable packets sent to this client, waiting for their ACK. */
  ReliableOutbox outbox;
};

/** @brief What a SessionListener is told about an address. */
enum class SessionChange : std::uint8_t {
  /** @brief The address is now a client of the game. */
  Joined,
  /** @brief The address is not a client anymore: it left, was dropped, or
   *         was refused. */
  Left,
};

/**
 * @brief Called by a GameServer, from its own thread, each time a client
 *        joins or leaves it.
 */
using SessionListener =
    std::function<void(const ampersand::net::Endpoint&, SessionChange)>;

/** @brief How the Lobby knows a GameServer. */
struct PartyLink {
  /** @brief Number of the party, shown in the logs. */
  std::uint32_t id{};
  /** @brief Told about every client that joins or leaves, if set. */
  SessionListener listener;
};

/**
 * @brief One party of the server: the link between its clients and its Game.
 *
 * The Lobby gives it the packets of its clients with push(); each party runs
 * in its own thread. run() calls step() SERVER_TICK_RATE times per second,
 * whether clients send something or not. Each step handles the packets
 * pushed since the last one (joins, inputs, ACKs...), runs one tick of the
 * Game, sends its
 * events to every client, and sends a WORLD_SNAPSHOT SNAPSHOT_RATE times per
 * second. Reliable packets without ACK are sent again; a client that stays
 * silent, or never acknowledges them, is removed and the others are told.
 */
class GameServer {
 public:
  using Clock = std::chrono::steady_clock;

  /** @brief Time between two ticks (1/60 s). */
  static constexpr Clock::duration TICK_DURATION =
      std::chrono::duration_cast<Clock::duration>(std::chrono::seconds(1)) /
      protocol::SERVER_TICK_RATE;
  /** @brief A snapshot is sent every SNAPSHOT_INTERVAL ticks. */
  static constexpr std::uint32_t SNAPSHOT_INTERVAL =
      protocol::SERVER_TICK_RATE / protocol::SNAPSHOT_RATE;
  /** @brief Silence after which a client is removed. */
  static constexpr std::chrono::milliseconds CLIENT_TIMEOUT{
      protocol::TIMEOUT_MS};
  /** @brief DISCONNECT is not reliable, so it is sent this many times. */
  static constexpr std::uint32_t DISCONNECT_REPEAT = 3;

  /**
   * @brief Creates an empty game.
   *
   * @param network Where packets are sent; must outlive the game.
   * @param seed Seed of the random parts of the Game.
   * @param link Number of the party and who to tell about its clients.
   */
  explicit GameServer(ServerNetwork& network,
                      std::uint32_t seed = std::random_device{}(),
                      PartyLink link = {});

  /**
   * @brief Queues a packet for the next step(). Can be called from any
   *        thread.
   *
   * @param message The packet and its sender.
   * @return false if the queue is full and the packet was dropped.
   */
  bool push(IncomingMessage message);

  /**
   * @brief Runs a tick every TICK_DURATION until a stop is requested, then
   *        tells every client that the server is shutting down.
   */
  void run(const std::stop_token& stopToken);

  /**
   * @brief Runs one tick.
   *
   * @param now Current time, used for the client timeouts and the resends.
   */
  void step(Clock::time_point now);

  /** @brief Number of ticks run so far. */
  [[nodiscard]] std::uint32_t tick() const noexcept;

  /** @brief Number of clients in the game. */
  [[nodiscard]] std::size_t playerCount() const noexcept;

  /** @brief The game logic, to inspect it (tests). */
  [[nodiscard]] Game& game() noexcept;

  /**
   * @brief Whether a name is valid UTF-8 without control characters.
   *
   * @param name Player name, as sent in CONNECT_REQUEST.
   * @return true if the name can be used.
   */
  [[nodiscard]] static bool isValidName(std::string_view name);

 private:
  /**
   * @brief Late ticks are run at once to catch up, but only up to this lag:
   *        past it (debugger pause, overloaded machine), they are skipped.
   */
  static constexpr Clock::duration MAX_LAG = TICK_DURATION * 10;

  void handleMessages(Clock::time_point now);
  void handleMessage(const IncomingMessage& message, Clock::time_point now);
  void handleInput(ClientSession& session, const protocol::PlayerInput& input);
  void acceptPlayer(const ampersand::net::Endpoint& from,
                    const std::string& name, Clock::time_point now);
  /** @brief Sends a new client everything it needs to join the game. */
  void welcome(ClientSession& session, Clock::time_point now);
  void removePlayer(std::uint8_t playerId, protocol::DisconnectReason reason,
                    Clock::time_point now);
  void dropSilentPlayers(Clock::time_point now);
  void resendReliable(Clock::time_point now);
  void broadcastEvents(const std::vector<GameEvent>& events,
                       Clock::time_point now);
  void broadcastSnapshot();
  void shutdown();

  /**
   * @brief Sends a payload to one client; a reliable one is kept until its
   *        ACK arrives.
   */
  void sendTo(ClientSession& session, const protocol::Payload& payload,
              Clock::time_point now);
  /** @brief sendTo() every client. */
  void broadcast(const protocol::Payload& payload, Clock::time_point now);

  /** @brief The packet that tells the clients about an event. */
  [[nodiscard]] protocol::Payload toPacket(const GameEvent& event) const;

  /** @brief The client playing from this address, or nullptr. */
  [[nodiscard]] ClientSession* findSession(
      const ampersand::net::Endpoint& endpoint);

  /**
   * @brief Tells the listener, if any.
   *
   * @param endpoint Address of the client that joined or left.
   * @param change Whether it joined or left.
   */
  void notify(const ampersand::net::Endpoint& endpoint, SessionChange change);

  /**
   * @brief Bytes of the UTF-8 character that starts a non-empty text, or 0 if
   *        it is invalid or a control character.
   */
  [[nodiscard]] static std::size_t characterLength(std::string_view text);
  /** @brief Name of a DisconnectReason, for logs. */
  [[nodiscard]] static std::string_view toString(
      protocol::DisconnectReason reason);

  ServerNetwork& _network;
  /** @brief Packets given by push(), handled by the next step(). */
  MessageQueue<IncomingMessage> _inbox{ServerNetwork::MAX_QUEUED_MESSAGES};
  /** @brief "[party N]", at the start of every log line. */
  std::string _logPrefix;
  /** @brief Told about every client that joins or leaves, if set. */
  SessionListener _listener;
  Game _game;
  /** @brief Indexed by player id; empty slots are free ids. */
  std::array<std::optional<ClientSession>, protocol::MAX_PLAYERS> _sessions;
  std::uint32_t _tick{0};
};

}  // namespace rtype::server
