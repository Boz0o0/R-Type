#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <rtype/common/protocol/Protocol.hpp>
#include <rtype/server/ReliableOutbox.hpp>
#include <span>
#include <vector>

using rtype::server::ReliableOutbox;

namespace {

const ReliableOutbox::Clock::time_point START = ReliableOutbox::Clock::now();

std::vector<std::byte> bytes(std::uint8_t value) {
  return {std::byte{value}, std::byte{value}};
}

/** @brief Runs resendDue() and counts what it sends. */
struct Resender {
  std::vector<std::vector<std::byte>> sent;

  bool operator()(ReliableOutbox& outbox,
                  ReliableOutbox::Clock::time_point now) {
    return outbox.resendDue(now, [this](std::span<const std::byte> datagram) {
      this->sent.emplace_back(datagram.begin(), datagram.end());
    });
  }
};

}  // namespace

TEST(ReliableOutbox, WaitsBeforeSendingAgain) {
  ReliableOutbox outbox;
  outbox.add(1, bytes(1), START);
  Resender resend;

  EXPECT_TRUE(resend(outbox, START + std::chrono::milliseconds(50)));
  EXPECT_TRUE(resend.sent.empty());

  EXPECT_TRUE(resend(outbox, START + ReliableOutbox::RESEND_INTERVAL));
  ASSERT_EQ(resend.sent.size(), 1U);
  EXPECT_EQ(resend.sent[0], bytes(1));  // the same bytes, same sequence
}

TEST(ReliableOutbox, StopsSendingOnceAcknowledged) {
  ReliableOutbox outbox;
  outbox.add(1, bytes(1), START);
  outbox.add(2, bytes(2), START);
  outbox.acknowledge(1);
  outbox.acknowledge(42);  // unknown: ignored
  Resender resend;

  EXPECT_TRUE(resend(outbox, START + ReliableOutbox::RESEND_INTERVAL));

  ASSERT_EQ(resend.sent.size(), 1U);
  EXPECT_EQ(resend.sent[0], bytes(2));
  EXPECT_EQ(outbox.size(), 1U);
}

TEST(ReliableOutbox, GivesUpAfterTheLastAttempt) {
  ReliableOutbox outbox;
  outbox.add(1, bytes(1), START);
  Resender resend;
  auto now = START;

  // The first send was done by add(), so MAX_SEND_ATTEMPTS - 1 resends.
  for (std::uint32_t attempt = 1; attempt < rtype::protocol::MAX_SEND_ATTEMPTS;
       ++attempt) {
    now += ReliableOutbox::RESEND_INTERVAL;
    ASSERT_TRUE(resend(outbox, now));
  }
  EXPECT_EQ(resend.sent.size(), rtype::protocol::MAX_SEND_ATTEMPTS - 1);

  now += ReliableOutbox::RESEND_INTERVAL;
  EXPECT_FALSE(resend(outbox, now));
}
