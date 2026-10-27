#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>

namespace rtype::client {

/** @brief Settings of the client, read from its command line. */
class ClientConfig {
 public:
  /**
   * @brief Reads the settings from the command line.
   *
   * @param args argv: the binary, then [host] [port] [name] [party code].
   * @return The settings, or std::nullopt if an argument is invalid or
   *         there are too many.
   */
  [[nodiscard]] static std::optional<ClientConfig> fromArguments(
      std::span<char* const> args);

  /** @brief Host name or address of the server. */
  [[nodiscard]] const std::string& host() const;
  /** @brief UDP port of the server. */
  [[nodiscard]] std::uint16_t port() const;
  /** @brief Name of the player. */
  [[nodiscard]] const std::string& name() const;
  /** @brief Code of the private party to join; empty for any public party. */
  [[nodiscard]] const std::string& partyCode() const;
  /** @brief assets/ next to the binary, found from the first argument. */
  [[nodiscard]] const std::filesystem::path& assetsDirectory() const;

 private:
  /**
   * @brief Reads a port number.
   *
   * @param text Port typed by the player.
   * @return The port, or std::nullopt if it is not 1 to 65535.
   */
  [[nodiscard]] static std::optional<std::uint16_t> parsePort(
      const std::string& text);

  /** @brief See host(). */
  std::string _host{"127.0.0.1"};
  /** @brief See port(). */
  std::uint16_t _port{4242};
  /** @brief See name(). */
  std::string _name{"Player"};
  /** @brief See partyCode(). */
  std::string _partyCode;
  /** @brief See assetsDirectory(). */
  std::filesystem::path _assetsDirectory{"assets"};
};

}  // namespace rtype::client
