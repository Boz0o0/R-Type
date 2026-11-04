#include <exception>
#include <iostream>
#include <mutex>
#include <optional>
#include <rtype/server/AuthWorker.hpp>
#include <rtype/server/Log.hpp>
#include <stop_token>
#include <utility>
#include <vector>

namespace rtype::server {

AuthWorker::AuthWorker(AccountStore& accounts)
    : _accounts(accounts), _thread([this](const std::stop_token& stopToken) {
        this->run(stopToken);
      }) {}

bool AuthWorker::submit(AuthJob job) {
  {
    const std::scoped_lock lock(this->_mutex);
    if (this->_jobs.size() >= MAX_PENDING) {
      return false;
    }
    this->_jobs.push_back(std::move(job));
  }
  this->_wake.notify_one();
  return true;
}

std::vector<AuthResult> AuthWorker::results() { return this->_results.drain(); }

void AuthWorker::run(const std::stop_token& stopToken) {
  while (true) {
    AuthJob job;
    {
      std::unique_lock lock(this->_mutex);
      // wait() also returns true when stopped with jobs left: check both.
      if (!this->_wake.wait(lock, stopToken,
                            [this] { return !this->_jobs.empty(); }) ||
          stopToken.stop_requested()) {
        return;
      }
      job = std::move(this->_jobs.front());
      this->_jobs.pop_front();
    }
    AuthResult result = this->check(job);
    if (!this->_results.push(std::move(result))) {
      logLine(std::cerr, "[auth] dropped the answer for ", job.username);
    }
  }
}

AuthResult AuthWorker::check(const AuthJob& job) {
  AuthResult result{
      .endpoint = job.endpoint,
      .username = job.username,
      .outcome = job.creates ? protocol::RejectReason::AccountExists
                             : protocol::RejectReason::BadCredentials,
  };
  try {
    std::optional<AccountStore::AccountId> account;
    if (job.creates) {
      account = this->_accounts.create(job.username, job.password);
      if (account) {
        logLine(std::cout, "[auth] account ", job.username, " created");
      }
    }
    if (!account) {
      // A LOGIN, or a REGISTER sent again: the account may exist already.
      account = this->_accounts.authenticate(job.username, job.password);
    }
    if (account) {
      result.outcome = *account;
    }
  } catch (const std::exception& error) {
    logLine(std::cerr, "[auth] ", job.username, ": ", error.what());
    result.outcome = protocol::RejectReason::ServerFull;
  }
  return result;
}

}  // namespace rtype::server
