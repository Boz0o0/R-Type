#include <charconv>
#include <rtype/client/ClientConfig.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <system_error>

namespace rtype::client {

std::optional<ClientConfig> ClientConfig::fromArguments(
    std::span<char* const> args) {
  ClientConfig config;
  if (args.size() > 4) {
    return std::nullopt;
  }
  if (args.size() > 1) {
    config._host = args[1];
    if (config._host.empty()) {
      return std::nullopt;
    }
  }
  if (args.size() > 2) {
    const auto port = parsePort(args[2]);
    if (!port) {
      return std::nullopt;
    }
    config._port = *port;
  }
  if (args.size() > 3) {
    config._name = args[3];
    if (config._name.empty() ||
        config._name.size() > protocol::MAX_PLAYER_NAME_LENGTH) {
      return std::nullopt;
    }
  }
  return config;
}

const std::string& ClientConfig::host() const { return this->_host; }
std::uint16_t ClientConfig::port() const { return this->_port; }
const std::string& ClientConfig::name() const { return this->_name; }

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
