#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <rtype/client/ClientConfig.hpp>
#include <string>
#include <utility>
#include <vector>

using rtype::client::ClientConfig;
using HostAndPort = std::pair<std::string, std::uint16_t>;

static std::optional<HostAndPort> parse(std::vector<std::string> words) {
  std::vector<char*> args;
  args.reserve(words.size());
  for (std::string& word : words) {
    args.push_back(word.data());
  }
  const auto config = ClientConfig::fromArguments(args);
  if (!config) {
    return std::nullopt;
  }
  return HostAndPort{config->host(), config->port()};
}

TEST(ClientConfig, UsesDefaultsWithoutArguments) {
  EXPECT_EQ(parse({"r-type_client"}), HostAndPort("127.0.0.1", 4242));
}

TEST(ClientConfig, ReadsHostOnly) {
  EXPECT_EQ(parse({"r-type_client", "10.0.0.2"}),
            HostAndPort("10.0.0.2", 4242));
}

TEST(ClientConfig, ReadsHostAndPort) {
  EXPECT_EQ(parse({"r-type_client", "10.0.0.2", "5000"}),
            HostAndPort("10.0.0.2", 5000));
}

TEST(ClientConfig, RejectsInvalidPorts) {
  for (const char* port : {"0", "abc", "42x", "70000", "-1", ""}) {
    EXPECT_FALSE(parse({"r-type_client", "localhost", port}).has_value())
        << "port: \"" << port << '"';
  }
}

TEST(ClientConfig, RejectsEmptyHost) {
  EXPECT_FALSE(parse({"r-type_client", ""}).has_value());
}

TEST(ClientConfig, RejectsExtraArguments) {
  EXPECT_FALSE(
      parse({"r-type_client", "localhost", "4242", "extra"}).has_value());
}
