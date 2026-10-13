#include <array>
#include <asio.hpp>
#include <cstdint>
#include <iostream>
#include <string>

int32_t main(int32_t argc, char** argv) {
  try {
    const uint16_t port =
        argc > 1 ? static_cast<uint16_t>(std::stoi(argv[1])) : 4242;

    asio::io_context ioContext;
    asio::ip::udp::socket socket(
        ioContext, asio::ip::udp::endpoint(asio::ip::udp::v4(), port));
    std::cout << "[server] listening on UDP port " << port << '\n';

    std::array<char, 1024> buffer{};
    while (true) {
      asio::ip::udp::endpoint client;
      const std::size_t len = socket.receive_from(asio::buffer(buffer), client);
      std::cout << "[server] received from " << client << ": "
                << std::string(buffer.data(), len) << '\n';
      socket.send_to(asio::buffer(std::string("ok")), client);
    }
  } catch (const std::exception& e) {
    std::cerr << "[server] error: " << e.what() << '\n';
    return 84;
  }
  return 0;
}
