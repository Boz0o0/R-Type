#include <ampersand/net/UdpSocket.hpp>
#include <atomic>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <iostream>
#include <rtype/common/protocol/Protocol.hpp>
#include <rtype/server/GameServer.hpp>
#include <rtype/server/ServerNetwork.hpp>
#include <string>

static_assert(std::atomic<bool>::is_always_lock_free,
              "the signal handler needs a lock-free flag");

static std::atomic<bool> stopRequested{false};

extern "C" void requestStop(int /*signal*/) { stopRequested = true; }

int32_t main(int32_t argc, char** argv) {
  using rtype::server::GameServer;
  using rtype::server::ServerNetwork;

  try {
    const uint16_t port = argc > 1 ? static_cast<uint16_t>(std::stoi(argv[1]))
                                   : rtype::protocol::DEFAULT_PORT;

    ServerNetwork network(port);
    GameServer game(network);
    std::cout << "[server] listening on UDP port " << network.localPort()
              << '\n';

    std::signal(SIGINT, requestStop);
    std::signal(SIGTERM, requestStop);
    game.run(stopRequested);
    std::cout << "[server] stopped\n";
  } catch (const std::exception& e) {
    std::fprintf(stderr, "[server] error: %s\n", e.what());
    return 84;
  } catch (...) {
    std::fprintf(stderr, "[server] unknown error\n");
    return 84;
  }
  return 0;
}
