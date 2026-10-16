#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>

namespace rtype::client {

class ClientConfig {
 public:
  [[nodiscard]] static std::optional<ClientConfig> fromArguments(
      std::span<char* const> args);

  [[nodiscard]] const std::string& host() const;
  [[nodiscard]] std::uint16_t port() const;

 private:
  [[nodiscard]] static std::optional<std::uint16_t> parsePort(
      const std::string& text);

  std::string _host{"127.0.0.1"};
  std::uint16_t _port{4242};
};

}  // namespace rtype::client
