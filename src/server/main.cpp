#include <ampersand/net/UdpSocket.hpp>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>
#include <thread>

using ampersand::net::Endpoint;
using ampersand::net::UdpSocket;

int32_t main(int32_t argc, char** argv) {
  try {
    const uint16_t port =
        argc > 1 ? static_cast<uint16_t>(std::stoi(argv[1])) : 4242;

    UdpSocket socket(port);
    std::cout << "[server] listening on UDP port " << socket.localPort()
              << '\n';

    std::array<std::byte, 1024> buffer{};
    Endpoint client;
    while (true) {
      while (const auto length = socket.receive(buffer, client)) {
        std::cout << "[server] received from " << client.toString() << ": "
                  << std::string(reinterpret_cast<const char*>(buffer.data()),
                                 *length)
                  << '\n';
        socket.send("ok", client);
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  } catch (const std::exception& e) {
    std::fprintf(stderr, "[server] error: %s\n", e.what());
    return 84;
  } catch (...) {
    std::fprintf(stderr, "[server] unknown error\n");
    return 84;
  }
  return 0;
}
