#include <gtest/gtest.h>

#include <cstddef>
#include <rtype/server/MessageQueue.hpp>
#include <thread>
#include <vector>

using rtype::server::MessageQueue;

TEST(MessageQueue, DrainReturnsMessagesInOrderAndEmptiesTheQueue) {
  MessageQueue<int> queue(8);
  EXPECT_TRUE(queue.push(1));
  EXPECT_TRUE(queue.push(2));
  EXPECT_TRUE(queue.push(3));
  EXPECT_EQ(queue.drain(), (std::vector<int>{1, 2, 3}));
  EXPECT_TRUE(queue.drain().empty());
}

TEST(MessageQueue, DropsMessagesWhenFull) {
  MessageQueue<int> queue(2);
  EXPECT_TRUE(queue.push(1));
  EXPECT_TRUE(queue.push(2));
  EXPECT_FALSE(queue.push(3));
  EXPECT_EQ(queue.drain(), (std::vector<int>{1, 2}));
  EXPECT_TRUE(queue.push(4));
}

TEST(MessageQueue, NoMessageIsLostBetweenTwoThreads) {
  constexpr int COUNT = 100000;
  MessageQueue<int> queue(COUNT);
  std::vector<int> received;

  std::jthread producer([&queue] {
    for (int value = 0; value < COUNT; ++value) {
      ASSERT_TRUE(queue.push(value));
    }
  });
  while (received.size() < static_cast<std::size_t>(COUNT)) {
    for (const int value : queue.drain()) {
      received.push_back(value);
    }
  }
  producer.join();

  for (int value = 0; value < COUNT; ++value) {
    ASSERT_EQ(received[static_cast<std::size_t>(value)], value);
  }
}
