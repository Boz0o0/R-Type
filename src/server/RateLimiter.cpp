#include <ampersand/net/UdpSocket.hpp>
#include <iostream>
#include <rtype/server/Log.hpp>
#include <rtype/server/RateLimiter.hpp>

namespace rtype::server {

RateLimiter::RateLimiter(Limits limits) : _limits(limits) {}

bool RateLimiter::allow(const ampersand::net::Endpoint& sender,
                        Clock::time_point now) {
  if (now - this->_windowStart >= WINDOW) {
    this->_windowStart = now;
    this->_counts.clear();
  }
  const std::string key = sender.toString();
  auto found = this->_counts.find(key);
  if (found == this->_counts.end()) {
    if (this->_counts.size() >= this->_limits.senders) {
      return false;
    }
    found = this->_counts.emplace(key, 0).first;
  }
  ++found->second;
  if (found->second == this->_limits.packetsPerSecond + 1) {
    logLine(std::cerr, "[server] ", key,
            " sends too much, dropping its packets");
  }
  return found->second <= this->_limits.packetsPerSecond;
}

}  // namespace rtype::server
