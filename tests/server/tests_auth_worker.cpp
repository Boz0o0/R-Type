#include <gtest/gtest.h>

#include <chrono>
#include <optional>
#include <rtype/common/protocol/Protocol.hpp>
#include <rtype/server/AccountStore.hpp>
#include <rtype/server/AuthWorker.hpp>
#include <thread>
#include <variant>
#include <vector>

using rtype::protocol::RejectReason;
using rtype::server::AccountStore;
using rtype::server::AuthJob;
using rtype::server::AuthResult;
using rtype::server::AuthWorker;

namespace {

/** @brief Waits for the next answer of the worker. */
std::optional<AuthResult> nextResult(AuthWorker& worker) {
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (std::chrono::steady_clock::now() < deadline) {
    std::vector<AuthResult> results = worker.results();
    if (!results.empty()) {
      return std::move(results.front());
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return std::nullopt;
}

AuthJob job(const std::string& username, const std::string& password,
            bool creates) {
  return AuthJob{
      .endpoint = {.address = "127.0.0.1", .port = 1234},
      .username = username,
      .password = password,
      .creates = creates,
  };
}

}  // namespace

TEST(AuthWorker, CreatesThenLogsIn) {
  AccountStore accounts(":memory:");
  AuthWorker worker(accounts);

  ASSERT_TRUE(worker.submit(job("Alice", "pw", true)));
  const auto created = nextResult(worker);
  ASSERT_TRUE(created.has_value());
  ASSERT_TRUE(
      std::holds_alternative<AccountStore::AccountId>(created->outcome));

  ASSERT_TRUE(worker.submit(job("Alice", "pw", false)));
  const auto logged = nextResult(worker);
  ASSERT_TRUE(logged.has_value());
  EXPECT_EQ(logged->outcome, created->outcome);
  EXPECT_EQ(logged->username, "Alice");
  EXPECT_EQ(logged->endpoint.port, 1234);
}

TEST(AuthWorker, RefusesAWrongPassword) {
  AccountStore accounts(":memory:");
  ASSERT_TRUE(accounts.create("Alice", "pw").has_value());
  AuthWorker worker(accounts);

  ASSERT_TRUE(worker.submit(job("Alice", "nope", false)));
  const auto result = nextResult(worker);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(std::get<RejectReason>(result->outcome),
            RejectReason::BadCredentials);
}

TEST(AuthWorker, RefusesATakenUsername) {
  AccountStore accounts(":memory:");
  ASSERT_TRUE(accounts.create("Alice", "pw").has_value());
  AuthWorker worker(accounts);

  ASSERT_TRUE(worker.submit(job("Alice", "other", true)));
  const auto result = nextResult(worker);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(std::get<RejectReason>(result->outcome),
            RejectReason::AccountExists);
}

TEST(AuthWorker, RefusesJobsPastTheLimit) {
  AccountStore accounts(":memory:");
  AuthWorker worker(accounts);

  std::size_t accepted = 0;
  while (accepted <= AuthWorker::MAX_PENDING + 1 &&
         worker.submit(job("Ghost", "pw", false))) {
    ++accepted;
  }

  // One may already be running, out of the queue.
  EXPECT_LE(accepted, AuthWorker::MAX_PENDING + 1);
}

TEST(AuthWorker, StopsWithJobsLeft) {
  AccountStore accounts(":memory:");
  const auto hashStart = std::chrono::steady_clock::now();
  (void)accounts.authenticate("Ghost", "pw");
  const auto oneHash = std::chrono::steady_clock::now() - hashStart;
  constexpr int JOBS = 10;

  const auto start = std::chrono::steady_clock::now();
  {
    AuthWorker worker(accounts);
    for (int index = 0; index < JOBS; ++index) {
      ASSERT_TRUE(worker.submit(job("Ghost", "pw", false)));
    }
  }  // waits for the running hash only

  EXPECT_LT(std::chrono::steady_clock::now() - start, oneHash * (JOBS / 2));
}
