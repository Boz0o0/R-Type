#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <rtype/common/protocol/PacketCodec.hpp>
#include <rtype/common/protocol/Packets.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <rtype/server/AccountStore.hpp>
#include <rtype/server/Lobby.hpp>
#include <rtype/server/LoginLimiter.hpp>
#include <rtype/server/ServerNetwork.hpp>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#include "ServerTestHelpers.hpp"

using namespace rtype::protocol;
using rtype::server::AccountStore;
using rtype::server::Lobby;
using rtype::server::ServerNetwork;
using rtype::server::test::payloadsOf;
using rtype::server::test::TestClient;

namespace {

/** @brief Argon2 is slow on purpose, even more in a Debug build. */
constexpr std::chrono::milliseconds AUTH_WAIT{5000};
constexpr std::chrono::milliseconds NO_ANSWER_WAIT{300};

/** @brief Sends a request and runs the lobby until the client gets a T. */
template <typename T>
std::optional<T> ask(Lobby& lobby, TestClient& client, const Payload& request) {
  client.send(request);
  const auto deadline = std::chrono::steady_clock::now() + AUTH_WAIT;
  while (std::chrono::steady_clock::now() < deadline) {
    lobby.step();
    while (const auto packet = client.poll()) {
      if (const auto* payload = std::get_if<T>(&packet->payload)) {
        return *payload;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return std::nullopt;
}

/** @brief Every packet the client receives for a while, the lobby running. */
std::vector<Packet> collect(Lobby& lobby, TestClient& client) {
  const auto deadline = std::chrono::steady_clock::now() + NO_ANSWER_WAIT;
  std::vector<Packet> packets;
  while (std::chrono::steady_clock::now() < deadline) {
    lobby.step();
    while (auto packet = client.poll()) {
      packets.push_back(std::move(*packet));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return packets;
}

/** @brief A lobby with accounts, and its database in memory. */
struct AccountLobby {
  ServerNetwork network{0};
  AccountStore accounts{":memory:"};
  Lobby lobby{network, accounts};
};

}  // namespace

TEST(LobbyAccounts, RegisterEntersTheLobby) {
  AccountLobby server;
  TestClient client(server.network);

  const auto accept = ask<ConnectAccept>(
      server.lobby, client, Register{.username = "Alice", .password = "pw"});

  ASSERT_TRUE(accept.has_value());
  EXPECT_EQ(server.lobby.visitorCount(), 1U);
  EXPECT_TRUE(server.accounts.authenticate("Alice", "pw").has_value());
}

TEST(LobbyAccounts, LoginEntersTheLobby) {
  AccountLobby server;
  ASSERT_TRUE(server.accounts.create("Alice", "pw").has_value());
  TestClient client(server.network);

  const auto accept = ask<ConnectAccept>(
      server.lobby, client, Login{.username = "Alice", .password = "pw"});

  ASSERT_TRUE(accept.has_value());
  EXPECT_EQ(server.lobby.visitorCount(), 1U);
}

TEST(LobbyAccounts, RejectsAWrongPassword) {
  AccountLobby server;
  ASSERT_TRUE(server.accounts.create("Alice", "pw").has_value());
  TestClient client(server.network);

  const auto reject = ask<ConnectReject>(
      server.lobby, client, Login{.username = "Alice", .password = "nope"});

  ASSERT_TRUE(reject.has_value());
  EXPECT_EQ(reject->reason, RejectReason::BadCredentials);
  EXPECT_EQ(server.lobby.visitorCount(), 0U);
}

TEST(LobbyAccounts, RejectsAnUnknownUserLikeAWrongPassword) {
  AccountLobby server;
  TestClient client(server.network);

  const auto reject = ask<ConnectReject>(
      server.lobby, client, Login{.username = "Ghost", .password = "pw"});

  ASSERT_TRUE(reject.has_value());
  EXPECT_EQ(reject->reason, RejectReason::BadCredentials);
}

TEST(LobbyAccounts, RejectsATakenUsername) {
  AccountLobby server;
  ASSERT_TRUE(server.accounts.create("Alice", "pw").has_value());
  TestClient client(server.network);

  const auto reject = ask<ConnectReject>(
      server.lobby, client, Register{.username = "alice", .password = "other"});

  ASSERT_TRUE(reject.has_value());
  EXPECT_EQ(reject->reason, RejectReason::AccountExists);
  EXPECT_EQ(server.lobby.visitorCount(), 0U);
}

TEST(LobbyAccounts, RejectsAnInvalidUsername) {
  AccountLobby server;
  TestClient client(server.network);

  const auto reject = ask<ConnectReject>(
      server.lobby, client, Register{.username = "Bad\x01", .password = "pw"});

  ASSERT_TRUE(reject.has_value());
  EXPECT_EQ(reject->reason, RejectReason::InvalidName);
}

TEST(LobbyAccounts, RejectsASecondLoginOfTheSameAccount) {
  AccountLobby server;
  ASSERT_TRUE(server.accounts.create("Alice", "pw").has_value());
  TestClient first(server.network);
  TestClient second(server.network);
  ASSERT_TRUE(ask<ConnectAccept>(server.lobby, first,
                                 Login{.username = "Alice", .password = "pw"})
                  .has_value());

  const auto reject = ask<ConnectReject>(
      server.lobby, second, Login{.username = "Alice", .password = "pw"});

  ASSERT_TRUE(reject.has_value());
  EXPECT_EQ(reject->reason, RejectReason::AlreadyLoggedIn);
  EXPECT_EQ(server.lobby.visitorCount(), 1U);
}

// The CONNECT_ACCEPT of the first copy was lost: the client sends it again.
TEST(LobbyAccounts, AnswersARepeatedRegisterOrLogin) {
  AccountLobby server;
  TestClient client(server.network);
  const Register request{.username = "Alice", .password = "pw"};
  ASSERT_TRUE(ask<ConnectAccept>(server.lobby, client, request).has_value());

  EXPECT_TRUE(ask<ConnectAccept>(server.lobby, client, request).has_value());
  EXPECT_TRUE(ask<ConnectAccept>(server.lobby, client,
                                 Login{.username = "Alice", .password = "pw"})
                  .has_value());
  EXPECT_EQ(server.lobby.visitorCount(), 1U);
}

TEST(LobbyAccounts, IgnoresConnectRequest) {
  AccountLobby server;
  TestClient client(server.network);

  client.send(ConnectRequest{.playerName = "Alice"});
  const auto packets = collect(server.lobby, client);

  EXPECT_TRUE(packets.empty());
  EXPECT_EQ(server.lobby.visitorCount(), 0U);
}

TEST(LobbyAccounts, IgnoresConnectRequestFromALoggedInClient) {
  AccountLobby server;
  TestClient client(server.network);
  ASSERT_TRUE(
      ask<ConnectAccept>(server.lobby, client,
                         Register{.username = "Alice", .password = "pw"})
          .has_value());

  // Without a password, it may not change its name.
  client.send(ConnectRequest{.playerName = "Mallory"});
  const auto packets = collect(server.lobby, client);

  EXPECT_TRUE(payloadsOf<ConnectAccept>(packets).empty());
  EXPECT_EQ(server.lobby.visitorCount(), 1U);
}

TEST(LobbyAccounts, PlaysUnderItsUsername) {
  AccountLobby server;
  TestClient client(server.network);
  ASSERT_TRUE(
      ask<ConnectAccept>(server.lobby, client,
                         Register{.username = "Alice", .password = "pw"})
          .has_value());

  const auto joined = ask<PlayerJoined>(server.lobby, client, RoomCreate{});

  ASSERT_TRUE(joined.has_value());
  EXPECT_EQ(joined->playerName, "Alice");
}

TEST(LobbyAccounts, RefusesAnAddressThatFailsTooOften) {
  AccountLobby server;
  ASSERT_TRUE(server.accounts.create("Alice", "pw").has_value());
  TestClient client(server.network);
  for (std::uint32_t attempt = 0;
       attempt < rtype::server::LoginLimiter::MAX_ATTEMPTS; ++attempt) {
    const auto reject = ask<ConnectReject>(
        server.lobby, client, Login{.username = "Alice", .password = "bad"});
    ASSERT_TRUE(reject.has_value());
    ASSERT_EQ(reject->reason, RejectReason::BadCredentials);
  }

  // Even the right password, from any port of this address.
  TestClient other(server.network);
  const auto reject = ask<ConnectReject>(
      server.lobby, other, Login{.username = "Alice", .password = "pw"});

  ASSERT_TRUE(reject.has_value());
  EXPECT_EQ(reject->reason, RejectReason::TooManyAttempts);
}

TEST(LobbyAccounts, DoesNotCountTheSuccessfulLogins) {
  AccountLobby server;
  std::vector<std::unique_ptr<TestClient>> clients;
  for (std::uint32_t index = 0;
       index <= rtype::server::LoginLimiter::MAX_ATTEMPTS; ++index) {
    clients.push_back(std::make_unique<TestClient>(server.network));
    const std::string name = "Player" + std::to_string(index);
    ASSERT_TRUE(ask<ConnectAccept>(server.lobby, *clients.back(),
                                   Register{.username = name, .password = "pw"})
                    .has_value())
        << name;
  }
}

TEST(LobbyAccounts, IgnoresCopiesWhileThePasswordIsChecked) {
  AccountLobby server;
  ASSERT_TRUE(server.accounts.create("Alice", "pw").has_value());
  TestClient client(server.network);

  // Many copies of a wrong login: one attempt only.
  for (int copy = 0; copy < 20; ++copy) {
    client.send(Login{.username = "Alice", .password = "bad"});
  }
  const auto packets = collect(server.lobby, client);
  std::size_t rejects = payloadsOf<ConnectReject>(packets).size();
  const auto deadline = std::chrono::steady_clock::now() + AUTH_WAIT;
  while (rejects == 0 && std::chrono::steady_clock::now() < deadline) {
    rejects += payloadsOf<ConnectReject>(collect(server.lobby, client)).size();
  }
  EXPECT_GE(rejects, 1U);

  // So the right password still works.
  EXPECT_TRUE(ask<ConnectAccept>(server.lobby, client,
                                 Login{.username = "Alice", .password = "pw"})
                  .has_value());
}

// The point of the AuthWorker: a slow hash does not stop the lobby.
TEST(LobbyAccounts, AnswersOtherClientsWhileAPasswordIsChecked) {
  AccountLobby server;
  TestClient alice(server.network);
  ASSERT_TRUE(
      ask<ConnectAccept>(server.lobby, alice,
                         Register{.username = "Alice", .password = "pw"})
          .has_value());
  const auto hashStart = std::chrono::steady_clock::now();
  (void)server.accounts.authenticate("Nobody", "pw");
  const auto oneHash = std::chrono::steady_clock::now() - hashStart;

  TestClient bob(server.network);
  bob.send(Register{.username = "Bob", .password = "pw"});
  std::this_thread::sleep_for(std::chrono::milliseconds(5));
  const auto start = std::chrono::steady_clock::now();
  const auto pong = ask<Pong>(server.lobby, alice, Ping{.timestampMs = 7});

  ASSERT_TRUE(pong.has_value());
  EXPECT_LT(std::chrono::steady_clock::now() - start, oneHash / 2);
}
