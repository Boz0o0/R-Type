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

  bool operator==(const ClientConfig&) const = default;

  std::string host{"127.0.0.1"};
  std::uint16_t port{4242};

 private:
  [[nodiscard]] static std::optional<std::uint16_t> parsePort(
      const std::string& text);
};

}  // namespace rtype::client
