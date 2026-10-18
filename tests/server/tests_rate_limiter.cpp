#include <gtest/gtest.h>

#include <ampersand/net/UdpSocket.hpp>
#include <chrono>
#include <cstdint>
#include <rtype/server/RateLimiter.hpp>

using ampersand::net::Endpoint;
using rtype::server::RateLimiter;

namespace {

const RateLimiter::Clock::time_point START = RateLimiter::Clock::now();

Endpoint alice() { return {.address = "10.0.0.1", .port = 5000}; }
Endpoint bob() { return {.address = "10.0.0.2", .port = 5000}; }

}  // namespace

TEST(RateLimiter, DropsWhatASenderSendsPastTheLimit) {
  RateLimiter limiter({.packetsPerSecond = 3, .senders = 16});

  for (int count = 0; count < 3; ++count) {
    EXPECT_TRUE(limiter.allow(alice(), START));
  }
  EXPECT_FALSE(limiter.allow(alice(), START));
  EXPECT_TRUE(limiter.allow(bob(), START));  // each sender has its own count
}

TEST(RateLimiter, StartsCountingAgainEverySecond) {
  RateLimiter limiter({.packetsPerSecond = 1, .senders = 16});
  ASSERT_TRUE(limiter.allow(alice(), START));
  ASSERT_FALSE(limiter.allow(alice(), START + std::chrono::milliseconds(999)));

  EXPECT_TRUE(limiter.allow(alice(), START + RateLimiter::WINDOW));
}

TEST(RateLimiter, CountsALimitedNumberOfSenders) {
  RateLimiter limiter({.packetsPerSecond = 10, .senders = 1});
  ASSERT_TRUE(limiter.allow(alice(), START));

  EXPECT_FALSE(limiter.allow(bob(), START));
  EXPECT_TRUE(limiter.allow(bob(), START + RateLimiter::WINDOW));
}
