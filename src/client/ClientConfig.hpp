#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>

namespace rtype::client {

struct ClientConfig {
  std::string host{"127.0.0.1"};
  std::uint16_t port{4242};

  bool operator==(const ClientConfig&) const = default;
};

[[nodiscard]] std::optional<ClientConfig> parseArguments(
    std::span<char* const> args);

}  // namespace rtype::client
