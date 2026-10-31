#pragma once

#include <ampersand/net/UdpSocket.hpp>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <rtype/common/protocol/Packets.hpp>
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
 * @brief The lobby of the server: its clients, and the parties they play in.
 *
 * A CONNECT_REQUEST from a new address makes it a client of the lobby. A
 * client in no party (a visitor) talks to the Lobby itself: it asks for the
 * list of the public parties (ROOM_LIST), opens a party (ROOM_CREATE) or
 * joins one (ROOM_JOIN), and is dropped after CLIENT_TIMEOUT of silence.
 *
 * Every party is a GameServer with its own thread and its own tick loop. The
 * Lobby gives each packet of a client in a party to that party, which then
 * answers it, keeps it alive and times it out. A client that sends
 * ROOM_LEAVE comes back to the lobby; one that quits or is dropped leaves
 * the server.
 *
 * ROOM_JOIN with a party code goes to the private party of that code (letter
 * case ignored), opened if there is none yet: only the clients who know the
 * code play in it. Without code, it goes to the public party asked, or to
 * the first public party with a free place, opened if they are all full.
 * ROOM_CREATE always opens a party; a private one gets a code made of
 * GENERATED_CODE_LENGTH digits. The Lobby never runs more parties than the
 * limit it was given. A party is closed as soon as its last client is gone,
 * and its code can then be used again.
 *
 * The Lobby is the only one to touch its tables. The parties tell it who
 * joined or left through a MessageQueue, and it reads that queue before
 * routing new packets: so no mutex is needed besides the ones of the queues.
 */
class Lobby {
 public:
  using Clock = std::chrono::steady_clock;
  /** @brief Number of a party, never given twice by the same Lobby. */
  using PartyId = std::uint32_t;

  /** @brief Parties run at once by default. */
  static constexpr std::size_t MAX_PARTIES = 16;
  /** @brief Clients at once, in the lobby or in a party. */
  static constexpr std::size_t MAX_CLIENTS = 256;
  /** @brief Joins and leaves waiting between two step() calls. */
  static constexpr std::size_t MAX_QUEUED_CHANGES = 4096;
  /** @brief Wait between two steps in run(). */
  static constexpr std::chrono::milliseconds IDLE_DELAY =
      std::chrono::milliseconds(1);
  /** @brief Silence after which a client in no party is dropped. */
  static constexpr std::chrono::milliseconds CLIENT_TIMEOUT{
      protocol::TIMEOUT_MS};
  /**
   * @brief Shortest delay between two ROOM_LIST_REPLY to the same client.
   *
   * The reply is bigger than the request: answering every request would let
   * someone who fakes the address of a client flood it with replies.
   */
  static constexpr std::chrono::milliseconds ROOM_LIST_COOLDOWN{250};
  /** @brief Digits of the code given to a private party by ROOM_CREATE. */
  static constexpr std::size_t GENERATED_CODE_LENGTH = 6;
  /** @brief DISCONNECT is not reliable, so it is sent this many times. */
  static constexpr std::uint32_t DISCONNECT_REPEAT = 3;

  /**
   * @brief Creates a lobby without any client nor party.
   *
   * @param network Where packets come from and go to; must outlive the lobby.
   * @param maxParties Most parties run at once; past it, ROOM_CREATE and
   *        ROOM_JOIN that need a new party are refused with ServerFull.
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
   *        party and tells every client that the server is shutting down.
   *
   * @param stopRequested Set to true, from any thread, to stop the lobby.
   */
  void run(const std::atomic<bool>& stopRequested);

  /** @brief step() at the current time. */
  void step();

  /**
   * @brief Takes the joins and leaves told by the parties, handles the
   *        packets received since the last call, drops the silent visitors
   *        and closes the empty parties.
   *
   * @param now Current time, used for the timeouts of the visitors.
   */
  void step(Clock::time_point now);

  /**
   * @brief Number of parties running.
   *
   * @return Parties opened and not closed yet.
   */
  [[nodiscard]] std::size_t partyCount() const noexcept;

  /**
   * @brief Number of clients in every party.
   *
   * @return Clients routed to a party, joined or about to.
   */
  [[nodiscard]] std::size_t playerCount() const noexcept;

  /**
   * @brief Number of clients in the lobby, in no party.
   *
   * @return Clients connected but not routed to a party.
   */
  [[nodiscard]] std::size_t visitorCount() const noexcept;

  /**
   * @brief The party of a client, if it is in one.
   *
   * @param endpoint Address of the client.
   * @return Its party, or std::nullopt if it is in none.
   */
  [[nodiscard]] std::optional<PartyId> partyOf(
      const ampersand::net::Endpoint& endpoint) const;

  /**
   * @brief The private party of a code, if one is running.
   *
   * @param code Party code, in any letter case.
   * @return Its party, or std::nullopt if none is running.
   */
  [[nodiscard]] std::optional<PartyId> partyWithCode(std::string code) const;

  /**
   * @brief A party code in upper case, the form used by the Lobby.
   *
   * @param code Party code, in any letter case.
   * @return The same code, its letters in upper case.
   */
  [[nodiscard]] static std::string normalizeCode(std::string code);

 private:
  /** @brief A client connected to the server. */
  struct Client {
    /** @brief Its address. */
    ampersand::net::Endpoint endpoint;
    /** @brief Name it gave in CONNECT_REQUEST. */
    std::string name;
    /** @brief The party its packets go to; std::nullopt in the lobby. */
    std::optional<PartyId> party;
    /** @brief When it last sent a packet while in the lobby. */
    Clock::time_point lastSeen;
    /** @brief When it was last sent a ROOM_LIST_REPLY. */
    std::optional<Clock::time_point> lastRoomList;
  };

  /** @brief A running GameServer and what the Lobby knows of it. */
  struct Party {
    /** @brief The game; on the heap, so that its address never changes. */
    std::unique_ptr<GameServer> server;
    /** @brief Code to join it, in upper case; empty for a public party. */
    std::string code;
    /** @brief Clients routed to it, joined or about to. */
    std::size_t players{};
    /**
     * @brief Runs the server. Last member: stopped (then joined) before the
     *        server is destroyed.
     */
    std::jthread thread;
  };

  /** @brief A join or a leave told by a party. */
  struct Change {
    /** @brief Party that told it. */
    PartyId party{};
    /** @brief Address of the client that joined or left. */
    ampersand::net::Endpoint endpoint;
    /** @brief Whether it joined, left, or went back to the lobby. */
    SessionChange change{};
  };

  /**
   * @brief Updates the routes with the joins and leaves told since.
   *
   * @param now Current time: a client back in the lobby is seen now.
   */
  void applyChanges(Clock::time_point now);
  /**
   * @brief Gives a packet to the party of its sender, handles it if the
   *        sender is in the lobby, or gives it to connect() if unknown.
   *
   * @param message The packet and its sender.
   * @param now Current time.
   */
  void route(IncomingMessage message, Clock::time_point now);
  /**
   * @brief Makes an unknown address that sent CONNECT_REQUEST a client of
   *        the lobby, or sends CONNECT_REJECT.
   *
   * @param message The packet and its sender; other packets are ignored.
   * @param now Current time.
   */
  void connect(const IncomingMessage& message, Clock::time_point now);
  /**
   * @brief Handles a packet of a client in the lobby.
   *
   * @param client The client; may be removed (DISCONNECT).
   * @param payload What it sent.
   * @param now Current time.
   */
  void handleVisitor(Client& client, const protocol::Payload& payload,
                     Clock::time_point now);
  /**
   * @brief Sends the public parties to a client, unless it got them less
   *        than ROOM_LIST_COOLDOWN ago.
   */
  void sendRoomList(Client& client, Clock::time_point now);
  /** @brief Opens a party for a client in the lobby and puts it in. */
  void create(Client& client, protocol::RoomVisibility visibility);
  /** @brief Puts a client in the lobby in the party it asked for. */
  void join(Client& client, const protocol::RoomJoin& request);
  /**
   * @brief Routes a client to a party and asks the party to add it.
   *
   * @param client A client in the lobby.
   * @param party The party it joins.
   */
  void admit(Client& client, PartyId party);
  /**
   * @brief The party for a code (empty: any public party), opened if needed.
   *
   * @param code Party code in upper case, or empty.
   * @return The party, or why the client cannot join any.
   */
  [[nodiscard]] std::variant<PartyId, protocol::RejectReason> place(
      const std::string& code);
  /**
   * @brief The public party of an id, if it has a free place.
   *
   * @param roomId Id sent in ROOM_JOIN.
   * @return The party, or why the client cannot join it.
   */
  [[nodiscard]] std::variant<PartyId, protocol::RejectReason> findPublic(
      PartyId roomId) const;
  /**
   * @brief Starts a new party in its own thread.
   *
   * @param code Party code in upper case; empty for a public party.
   * @return The new party.
   */
  [[nodiscard]] PartyId open(const std::string& code);
  /** @brief A code of GENERATED_CODE_LENGTH digits no running party has. */
  [[nodiscard]] std::string generateCode();
  /** @brief Drops the clients in the lobby silent for CLIENT_TIMEOUT. */
  void dropSilentVisitors(Clock::time_point now);
  /** @brief Stops the parties without any client and frees their codes. */
  void closeEmptyParties();
  /** @brief Tells the clients in the lobby that the server shuts down. */
  void sayGoodbyeToVisitors();

  /**
   * @brief Removes the route of a client, if it still goes to this party.
   *
   * @param client The client.
   * @param party Party it left.
   * @return true if the client was routed to this party.
   */
  bool unbind(Client& client, PartyId party);

  /** @brief The client of an address, or nullptr. */
  [[nodiscard]] Client* findClient(const ampersand::net::Endpoint& endpoint);

  /** @brief Shared by every party; owned by the caller. */
  ServerNetwork& _network;
  /** @brief Most parties run at once. */
  std::size_t _maxParties;
  /** @brief Joins and leaves pushed by the parties, read by step(). */
  MessageQueue<Change> _changes{MAX_QUEUED_CHANGES};
  /** @brief Every client, keyed by Endpoint::toString(). */
  std::unordered_map<std::string, Client> _clients;
  /** @brief Clients routed to a party. */
  std::size_t _players{0};
  /** @brief Private party of each code, in upper case. */
  std::unordered_map<std::string, PartyId> _codes;
  /**
   * @brief Running parties. After _changes: the parties, which push into it,
   *        are destroyed first.
   */
  std::map<PartyId, Party> _parties;
  /** @brief Number given to the next party opened. */
  PartyId _nextPartyId{1};
  /** @brief Draws the codes of the private parties of ROOM_CREATE. */
  std::mt19937 _random{std::random_device{}()};
};

}  // namespace rtype::server
