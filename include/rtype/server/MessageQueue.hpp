#pragma once

#include <cstddef>
#include <mutex>
#include <utility>
#include <vector>

namespace rtype::server {

/**
 * @brief A bounded queue shared by one thread that pushes and one that drains.
 *
 * Every access locks a std::mutex, held only for the time of a push_back or a
 * swap, so the network thread is never blocked long by the game thread.
 * When the queue is full, new messages are dropped, so a client flooding the
 * server cannot make it use all its memory.
 */
template <typename T>
class MessageQueue {
 public:
  /**
   * @brief Creates an empty queue.
   *
   * @param capacity Most messages kept between two drain() calls.
   */
  explicit MessageQueue(std::size_t capacity) : _capacity(capacity) {}

  /**
   * @brief Adds a message at the end of the queue.
   *
   * @return false if the queue is full and the message was dropped.
   */
  bool push(T message) {
    const std::scoped_lock lock(this->_mutex);
    if (this->_messages.size() >= this->_capacity) {
      return false;
    }
    this->_messages.push_back(std::move(message));
    return true;
  }

  /** @brief Takes every message pushed since the last call, oldest first. */
  [[nodiscard]] std::vector<T> drain() {
    std::vector<T> messages;
    const std::scoped_lock lock(this->_mutex);
    messages.swap(this->_messages);
    return messages;
  }

 private:
  std::size_t _capacity;
  std::mutex _mutex;
  std::vector<T> _messages;
};

}  // namespace rtype::server
