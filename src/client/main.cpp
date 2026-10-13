#include <array>
#include <asio.hpp>
#include <cstdint>
#include <iostream>
#include <string>

int32_t main(int32_t argc, char** argv) {
  try {
    const std::string host = argc > 1 ? argv[1] : "127.0.0.1";
    const std::string port = argc > 2 ? argv[2] : "4242";

    asio::io_context ioContext;
    asio::ip::udp::resolver resolver(ioContext);
    const asio::ip::udp::endpoint server =
        *resolver.resolve(asio::ip::udp::v4(), host, port).begin();

    asio::ip::udp::socket socket(ioContext);
    socket.open(asio::ip::udp::v4());

    socket.send_to(asio::buffer(std::string("hello")), server);
    std::cout << "[client] sent to " << server << ": hello\n";

    std::array<char, 1024> buffer{};
    asio::ip::udp::endpoint sender;
    const std::size_t len = socket.receive_from(asio::buffer(buffer), sender);
    std::cout << "[client] received from " << sender << ": "
              << std::string(buffer.data(), len) << '\n';
  } catch (const std::exception& e) {
    std::cerr << "[client] error: " << e.what() << '\n';
    return 84;
  }
  return 0;
}
