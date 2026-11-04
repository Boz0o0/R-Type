#include <rtype/server/LoginLimiter.hpp>
#include <string>
#include <unordered_map>

namespace rtype::server {

bool LoginLimiter::tryAttempt(const std::string& address,
                              Clock::time_point now) {
  auto found = this->_entries.find(address);
  if (found == this->_entries.end()) {
    if (this->_entries.size() >= MAX_ADDRESSES) {
      this->forgetExpired(now);
      if (this->_entries.size() >= MAX_ADDRESSES) {
        return false;
      }
    }
    found = this->_entries.emplace(address, Entry{.windowStart = now}).first;
  } else if (now - found->second.windowStart >= WINDOW) {
    found->second = Entry{.windowStart = now};
  }
  if (found->second.attempts >= MAX_ATTEMPTS) {
    return false;
  }
  ++found->second.attempts;
  return true;
}

void LoginLimiter::refund(const std::string& address) {
  const auto found = this->_entries.find(address);
  if (found != this->_entries.end() && found->second.attempts > 0) {
    --found->second.attempts;
  }
}

void LoginLimiter::forgetExpired(Clock::time_point now) {
  std::erase_if(this->_entries, [now](const auto& entry) {
    return now - entry.second.windowStart >= WINDOW;
  });
}

}  // namespace rtype::server
