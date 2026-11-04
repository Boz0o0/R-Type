#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <rtype/server/LoginLimiter.hpp>
#include <string>

using rtype::server::LoginLimiter;

namespace {

const LoginLimiter::Clock::time_point START = LoginLimiter::Clock::now();

void useEveryAttempt(LoginLimiter& limiter, const std::string& address) {
  for (std::uint32_t attempt = 0; attempt < LoginLimiter::MAX_ATTEMPTS;
       ++attempt) {
    ASSERT_TRUE(limiter.tryAttempt(address, START));
  }
}

}  // namespace

TEST(LoginLimiter, RefusesPastTheLimit) {
  LoginLimiter limiter;
  useEveryAttempt(limiter, "10.0.0.1");

  EXPECT_FALSE(limiter.tryAttempt("10.0.0.1", START));
}

TEST(LoginLimiter, CountsEachAddressApart) {
  LoginLimiter limiter;
  useEveryAttempt(limiter, "10.0.0.1");

  EXPECT_TRUE(limiter.tryAttempt("10.0.0.2", START));
}

TEST(LoginLimiter, AllowsAgainOnceTheWindowIsOver) {
  LoginLimiter limiter;
  useEveryAttempt(limiter, "10.0.0.1");

  EXPECT_FALSE(
      limiter.tryAttempt("10.0.0.1", START + LoginLimiter::WINDOW / 2));
  EXPECT_TRUE(limiter.tryAttempt("10.0.0.1", START + LoginLimiter::WINDOW));
}

TEST(LoginLimiter, DoesNotCountTheRefundedAttempts) {
  LoginLimiter limiter;
  for (std::uint32_t login = 0; login < 3 * LoginLimiter::MAX_ATTEMPTS;
       ++login) {
    ASSERT_TRUE(limiter.tryAttempt("10.0.0.1", START));
    limiter.refund("10.0.0.1");  // the password was right
  }
}

TEST(LoginLimiter, BoundsTheAddressesTracked) {
  LoginLimiter limiter;
  for (std::size_t index = 0; index < LoginLimiter::MAX_ADDRESSES; ++index) {
    ASSERT_TRUE(limiter.tryAttempt("addr" + std::to_string(index), START));
  }

  EXPECT_FALSE(limiter.tryAttempt("one more", START));
  // Once their window is over, the old addresses are forgotten.
  EXPECT_TRUE(limiter.tryAttempt("one more", START + LoginLimiter::WINDOW));
}
