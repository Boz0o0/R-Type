#pragma once

#include <ampersand/core/ecs/World.hpp>
#include <ampersand/net/UdpSocket.hpp>
#include <ampersand/physics/systems/MovementSystem.hpp>
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <rtype/common/protocol/Protocol.hpp>
#include <rtype/server/ServerNetwork.hpp>
#include <string>
#include <string_view>

namespace rtype::server {

/** @brief A client that joined the game. */
struct ClientSession {
  std::uint8_t playerId{};
  ampersand::net::Endpoint endpoint;
  std::string name;
  /** @brief When the last valid packet of this client arrived. */
  std::chrono::steady_clock::time_point lastSeen;
};

/**
 * @brief The game thread of the server.
 *
 * run() calls step() SERVER_TICK_RATE times per second, whether clients send
 * something or not. Each step handles the packets queued by the network
 * thread, updates the World, and sends a WORLD_SNAPSHOT to every client
 * SNAPSHOT_RATE times per second.
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

  /**
   * @brief Creates an empty game.
   *
   * @param network Where packets come from and go to; must outlive the game.
   */
  explicit GameServer(ServerNetwork& network);

  /**
   * @brief Runs a tick every TICK_DURATION until stopRequested becomes true,
   *        then tells every client that the server is shutting down.
   */
  void run(const std::atomic<bool>& stopRequested);

  /**
   * @brief Runs one tick.
   *
   * @param now Current time, used for the client timeouts.
   */
  void step(Clock::time_point now);

  /** @brief Number of ticks run so far. */
  [[nodiscard]] std::uint32_t tick() const noexcept;

  /** @brief Number of clients in the game. */
  [[nodiscard]] std::size_t playerCount() const noexcept;

 private:
  /**
   * @brief Late ticks are run at once to catch up, but only up to this lag:
   *        past it (debugger pause, overloaded machine), they are skipped.
   */
  static constexpr Clock::duration MAX_LAG = TICK_DURATION * 10;

  void handleMessages(Clock::time_point now);
  void handleMessage(const IncomingMessage& message, Clock::time_point now);
  void acceptPlayer(const ampersand::net::Endpoint& from,
                    const std::string& name, Clock::time_point now);
  void removePlayer(std::uint8_t playerId, std::string_view reason);
  void dropSilentPlayers(Clock::time_point now);
  void update();
  void broadcastSnapshot();
  void shutdown();

  /** @brief The client playing from this address, or nullptr. */
  [[nodiscard]] ClientSession* findSession(
      const ampersand::net::Endpoint& endpoint);

  ServerNetwork& _network;
  ampersand::core::ecs::World _world;
  ampersand::physics::MovementSystem _movementSystem;
  /** @brief Indexed by player id; empty slots are free ids. */
  std::array<std::optional<ClientSession>, protocol::MAX_PLAYERS> _sessions;
  std::uint32_t _tick{0};
};

}  // namespace rtype::server
