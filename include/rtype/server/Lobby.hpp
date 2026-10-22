#pragma once

#include <ampersand/net/UdpSocket.hpp>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <rtype/common/protocol/Protocol.hpp>
#include <rtype/server/GameServer.hpp>
#include <rtype/server/MessageQueue.hpp>
#include <rtype/server/ServerNetwork.hpp>
#include <string>
#include <thread>
#include <unordered_map>
#include <variant>

namespace rtype::server {

/**
 * @brief Runs several parties at once on one socket.
 *
 * Every party is a GameServer with its own thread and its own tick loop. The
 * Lobby takes the packets of the network thread and gives each one to the
 * party of its sender. A CONNECT_REQUEST from a new address without party
 * code goes to the first public party with a free place; when every public
 * party is full, a new one is opened. With a party code, it goes to the
 * private party of that code (letter case ignored), opened if there is none
 * yet: only the clients who know the code play in it. The Lobby never runs
 * more parties than the limit it was given. A party is closed as soon as its
 * last client is gone, and its code can then be used again.
 *
 * The Lobby is the only one to touch its tables. The parties tell it who
 * joined or left through a MessageQueue, and it reads that queue before
 * routing new packets: so no mutex is needed besides the ones of the queues.
 */
class Lobby {
 public:
  using PartyId = std::uint32_t;

  /** @brief Parties run at once by default. */
  static constexpr std::size_t MAX_PARTIES = 16;
  /** @brief Joins and leaves waiting between two step() calls. */
  static constexpr std::size_t MAX_QUEUED_CHANGES = 4096;
  /** @brief Wait between two steps in run(). */
  static constexpr std::chrono::milliseconds IDLE_DELAY{1};

  /**
   * @brief Creates a lobby without any party.
   *
   * @param network Where packets come from and go to; must outlive the lobby.
   * @param maxParties Most parties run at once; past it, new clients are
   *        refused with ServerFull.
   */
  explicit Lobby(ServerNetwork& network, std::size_t maxParties = MAX_PARTIES);

  /** @brief Stops every party and waits for their threads. */
  ~Lobby() = default;

  Lobby(const Lobby&) = delete;
  Lobby& operator=(const Lobby&) = delete;
  Lobby(Lobby&&) = delete;
  Lobby& operator=(Lobby&&) = delete;

  /**
   * @brief Runs step() until stopRequested becomes true, then stops every
   *        party (which tells its clients that the server is shutting down).
   */
  void run(const std::atomic<bool>& stopRequested);

  /**
   * @brief Takes the joins and leaves told by the parties, routes the packets
   *        received since the last call, and closes the empty parties.
   */
  void step();

  /** @brief Number of parties running. */
  [[nodiscard]] std::size_t partyCount() const noexcept;

  /** @brief Number of clients in every party. */
  [[nodiscard]] std::size_t playerCount() const noexcept;

  /** @brief The party of a client, if it is in one. */
  [[nodiscard]] std::optional<PartyId> partyOf(
      const ampersand::net::Endpoint& endpoint) const;

  /** @brief The private party of a code, if one is running. */
  [[nodiscard]] std::optional<PartyId> partyWithCode(std::string code) const;

  /** @brief A party code in upper case, the form used by the Lobby. */
  [[nodiscard]] static std::string normalizeCode(std::string code);

 private:
  struct Party {
    std::unique_ptr<GameServer> server;
    /** @brief Code to join it, in upper case; empty for a public party. */
    std::string code;
    /** @brief Clients routed to it, joined or about to. */
    std::size_t players{};
    // Last member: stopped (then joined) before the server is destroyed.
    std::jthread thread;
  };

  /** @brief A join or a leave told by a party. */
  struct Change {
    PartyId party{};
    ampersand::net::Endpoint endpoint;
    SessionChange change{};
  };

  void applyChanges();
  void route(IncomingMessage message);
  /** @brief Gives a CONNECT_REQUEST from an address in no party to one. */
  void connect(IncomingMessage message);
  /**
   * @brief The party for a code (empty: any public party), opened if needed.
   *
   * @return The party, or why the client cannot join any.
   */
  [[nodiscard]] std::variant<PartyId, protocol::RejectReason> place(
      const std::string& code);
  [[nodiscard]] PartyId open(const std::string& code);
  void closeEmptyParties();

  void bind(const ampersand::net::Endpoint& endpoint, PartyId party);
  void unbind(const ampersand::net::Endpoint& endpoint, PartyId party);

  ServerNetwork& _network;
  std::size_t _maxParties;
  MessageQueue<Change> _changes{MAX_QUEUED_CHANGES};
  /** @brief Party of each client, keyed by Endpoint::toString(). */
  std::unordered_map<std::string, PartyId> _routes;
  /** @brief Private party of each code, in upper case. */
  std::unordered_map<std::string, PartyId> _codes;
  // After _changes: the parties, which push into it, are destroyed first.
  std::map<PartyId, Party> _parties;
  PartyId _nextPartyId{1};
};

}  // namespace rtype::server
