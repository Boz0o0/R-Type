#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <vector>

#include "client/ClientConfig.hpp"

using rtype::client::ClientConfig;

static std::optional<ClientConfig> parse(std::vector<std::string> words) {
  std::vector<char*> args;
  args.reserve(words.size());
  for (std::string& word : words) {
    args.push_back(word.data());
  }
  return ClientConfig::fromArguments(args);
}

TEST(ClientConfig, UsesDefaultsWithoutArguments) {
  EXPECT_EQ(parse({"r-type_client"}), ClientConfig{});
}

TEST(ClientConfig, ReadsHostOnly) {
  const ClientConfig expected{.host = "10.0.0.2", .port = 4242};
  EXPECT_EQ(parse({"r-type_client", "10.0.0.2"}), expected);
}

TEST(ClientConfig, ReadsHostAndPort) {
  const ClientConfig expected{.host = "10.0.0.2", .port = 5000};
  EXPECT_EQ(parse({"r-type_client", "10.0.0.2", "5000"}), expected);
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
