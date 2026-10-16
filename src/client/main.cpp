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
    const std::string host = argc > 1 ? argv[1] : "127.0.0.1";
    const uint16_t port =
        argc > 2 ? static_cast<uint16_t>(std::stoi(argv[2])) : 4242;

    const Endpoint server = UdpSocket::resolve(host, port);
    UdpSocket socket;

    socket.send("hello", server);
    std::cout << "[client] sent to " << server.toString() << ": hello\n";

    // receive() never blocks, so poll until the reply arrives or we give up.
    std::array<std::byte, 1024> buffer{};
    Endpoint sender;
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (std::chrono::steady_clock::now() < deadline) {
      if (const auto length = socket.receive(buffer, sender)) {
        std::cout << "[client] received from " << sender.toString() << ": "
                  << std::string(reinterpret_cast<const char*>(buffer.data()),
                                 *length)
                  << '\n';
        return 0;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    std::fprintf(stderr, "[client] error: no reply from %s\n",
                 server.toString().c_str());
    return 84;
  } catch (const std::exception& e) {
    std::fprintf(stderr, "[client] error: %s\n", e.what());
    return 84;
  } catch (...) {
    std::fprintf(stderr, "[client] unknown error\n");
    return 84;
  }
}
