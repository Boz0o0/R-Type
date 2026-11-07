#include <ampersand/net/UdpSocket.hpp>
#include <atomic>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <iostream>
#include <rtype/common/protocol/Protocol.hpp>
#include <rtype/server/Game.hpp>
#include <rtype/server/Lobby.hpp>
#include <rtype/server/Log.hpp>
#include <rtype/server/ServerNetwork.hpp>
#include <stdexcept>
#include <string>

static_assert(std::atomic<bool>::is_always_lock_free,
              "the signal handler needs a lock-free flag");

static std::atomic<bool> stopRequested{false};

extern "C" void requestStop(int /*signal*/) { stopRequested = true; }

int32_t main(int32_t argc, char** argv) {
  using rtype::server::Game;
  using rtype::server::Lobby;
  using rtype::server::logLine;
  using rtype::server::ServerNetwork;

  try {
    const uint16_t port = argc > 1 ? static_cast<uint16_t>(std::stoi(argv[1]))
                                   : rtype::protocol::DEFAULT_PORT;

    if (!std::filesystem::is_regular_file(Game::CONFIG)) {
      throw std::runtime_error("cannot find " + std::string(Game::CONFIG) +
                               ", run the server from the repository or "
                               "from the folder of the client");
    }
    ServerNetwork network(port);
    rtype::server::AccountStore accounts("rtype.db");
    Lobby lobby(network, accounts);
    logLine(std::cout, "[server] listening on UDP port ", network.localPort());

    std::signal(SIGINT, requestStop);
    std::signal(SIGTERM, requestStop);
    lobby.run(stopRequested);
    logLine(std::cout, "[server] stopped");
  } catch (const std::exception& e) {
    std::fprintf(stderr, "[server] error: %s\n", e.what());
    return 84;
  } catch (...) {
    std::fprintf(stderr, "[server] unknown error\n");
    return 84;
  }
  return 0;
}
