#pragma once

#include <cstdint>
#include <filesystem>
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
  [[nodiscard]] const std::string& name() const;
  // Code of the private party to join; empty for any public party.
  [[nodiscard]] const std::string& partyCode() const;
  // assets/ next to the binary, found from the first argument.
  [[nodiscard]] const std::filesystem::path& assetsDirectory() const;

 private:
  [[nodiscard]] static std::optional<std::uint16_t> parsePort(
      const std::string& text);

  std::string _host{"127.0.0.1"};
  std::uint16_t _port{4242};
  std::string _name{"Player"};
  std::string _partyCode;
  std::filesystem::path _assetsDirectory{"assets"};
};

}  // namespace rtype::client
