#pragma once

#include <mutex>
#include <ostream>
#include <sstream>

namespace rtype::server {

/** @brief Lock shared by every logLine(), whatever the stream. */
inline std::mutex& logMutex() {
  static std::mutex mutex;
  return mutex;
}

/**
 * @brief Writes one line to a stream in a single piece.
 *
 * The network thread, the lobby and every game log at the same time: built
 * first and written under a lock, their lines never get mixed.
 */
template <typename... Parts>
void logLine(std::ostream& stream, const Parts&... parts) {
  std::ostringstream line;
  (line << ... << parts) << '\n';
  const std::scoped_lock lock(logMutex());
  stream << line.str() << std::flush;
}

}  // namespace rtype::server
