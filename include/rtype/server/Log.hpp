#pragma once

#include <mutex>
#include <ostream>
#include <sstream>

/**
 * @file Log.hpp
 * @brief Logging that stays readable when several threads write at once.
 */

namespace rtype::server {

/**
 * @brief Lock shared by every logLine(), whatever the stream.
 *
 * @return The same mutex on every call.
 */
inline std::mutex& logMutex() {
  static std::mutex mutex;
  return mutex;
}

/**
 * @brief Writes one line to a stream in a single piece.
 *
 * The network thread, the lobby and every game log at the same time: built
 * first and written under a lock, their lines never get mixed.
 *
 * @tparam Parts Types that can be written with operator<<.
 * @param stream Where to write, usually std::cout or std::cerr.
 * @param parts Pieces of the line, written one after the other; the line
 *        break is added.
 */
template <typename... Parts>
void logLine(std::ostream& stream, const Parts&... parts) {
  std::ostringstream line;
  (line << ... << parts) << '\n';
  const std::scoped_lock lock(logMutex());
  stream << line.str() << std::flush;
}

}  // namespace rtype::server
