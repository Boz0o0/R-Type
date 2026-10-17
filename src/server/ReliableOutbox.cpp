#include <cstddef>
#include <cstdint>
#include <rtype/common/protocol/Protocol.hpp>
#include <rtype/server/ReliableOutbox.hpp>
#include <utility>
#include <vector>

namespace rtype::server {

void ReliableOutbox::add(std::uint32_t sequence,
                         std::vector<std::byte> datagram,
                         Clock::time_point now) {
  this->_pending.push_back({
      .sequence = sequence,
      .datagram = std::move(datagram),
      .lastSent = now,
      .attempts = 1,
  });
}

void ReliableOutbox::acknowledge(std::uint32_t sequence) {
  std::erase_if(this->_pending, [sequence](const Pending& pending) {
    return pending.sequence == sequence;
  });
}

bool ReliableOutbox::resendDue(Clock::time_point now,
                               const SendFunction& send) {
  for (Pending& pending : this->_pending) {
    if (now - pending.lastSent < RESEND_INTERVAL) {
      continue;
    }
    if (pending.attempts >= protocol::MAX_SEND_ATTEMPTS) {
      return false;
    }
    send(pending.datagram);
    pending.lastSent = now;
    ++pending.attempts;
  }
  return true;
}

std::size_t ReliableOutbox::size() const noexcept {
  return this->_pending.size();
}

}  // namespace rtype::server
