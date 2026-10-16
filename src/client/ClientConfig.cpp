#include <charconv>
#include <rtype/client/ClientConfig.hpp>
#include <system_error>

namespace rtype::client {

std::optional<ClientConfig> ClientConfig::fromArguments(
    std::span<char* const> args) {
  ClientConfig config;
  if (args.size() > 3) {
    return std::nullopt;
  }
  if (args.size() > 1) {
    config.host = args[1];
    if (config.host.empty()) {
      return std::nullopt;
    }
  }
  if (args.size() > 2) {
    const auto port = parsePort(args[2]);
    if (!port) {
      return std::nullopt;
    }
    config.port = *port;
  }
  return config;
}

std::optional<std::uint16_t> ClientConfig::parsePort(const std::string& text) {
  std::uint16_t port = 0;
  const char* const end = text.data() + text.size();
  const auto [last, error] = std::from_chars(text.data(), end, port);
  if (error != std::errc{} || last != end || port == 0) {
    return std::nullopt;
  }
  return port;
}

}  // namespace rtype::client
