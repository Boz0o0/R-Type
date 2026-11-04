#pragma once

#include <ampersand/net/UdpSocket.hpp>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <rtype/common/protocol/Protocol.hpp>
#include <rtype/server/AccountStore.hpp>
#include <rtype/server/MessageQueue.hpp>
#include <stop_token>
#include <string>
#include <thread>
#include <variant>
#include <vector>

/**
 * @file AuthWorker.hpp
 * @brief Checks the passwords on a thread of its own.
 */

namespace rtype::server {

/** @brief A LOGIN or a REGISTER to check. */
struct AuthJob {
  /** @brief Sender, not a client of the lobby yet. */
  ampersand::net::Endpoint endpoint;
  std::string username;
  std::string password;
  /** @brief REGISTER: create the account first. */
  bool creates{false};
};

/** @brief The answer to an AuthJob. */
struct AuthResult {
  ampersand::net::Endpoint endpoint;
  std::string username;
  /** @brief The account, or why the login is refused. */
  std::variant<AccountStore::AccountId, protocol::RejectReason> outcome;
};

/**
 * @brief Runs the AccountStore on its own thread.
 *
 * Argon2 takes tens of milliseconds per password on purpose: done by the
 * Lobby, it would stop every client of the lobby meanwhile. The Lobby gives
 * the jobs with submit() and takes the answers with results() at each step,
 * so it never waits. Once given to a worker, the AccountStore is only used
 * by its thread.
 */
class AuthWorker {
 public:
  /** @brief Jobs waiting at once; past it, submit() refuses. */
  static constexpr std::size_t MAX_PENDING = 64;

  /**
   * @brief Starts the thread.
   *
   * @param accounts The accounts; must outlive the worker.
   */
  explicit AuthWorker(AccountStore& accounts);

  /** @brief Stops the thread; the jobs not started are dropped. */
  ~AuthWorker() = default;

  AuthWorker(const AuthWorker&) = delete;
  AuthWorker& operator=(const AuthWorker&) = delete;
  AuthWorker(AuthWorker&&) = delete;
  AuthWorker& operator=(AuthWorker&&) = delete;

  /**
   * @brief Queues a job.
   *
   * @return false if MAX_PENDING jobs are waiting already.
   */
  [[nodiscard]] bool submit(AuthJob job);

  /** @brief The answers ready since the last call. */
  [[nodiscard]] std::vector<AuthResult> results();

 private:
  void run(const std::stop_token& stopToken);
  /** @brief Checks a job against the accounts. */
  [[nodiscard]] AuthResult check(const AuthJob& job);

  AccountStore& _accounts;
  std::mutex _mutex;
  std::condition_variable_any _wake;
  /** @brief Guarded by _mutex. */
  std::deque<AuthJob> _jobs;
  /** @brief A job waits in _jobs or here, so MAX_PENDING always fits. */
  MessageQueue<AuthResult> _results{MAX_PENDING * 2};
  /** @brief Last member: stopped (then joined) before the rest goes. */
  std::jthread _thread;
};

}  // namespace rtype::server
