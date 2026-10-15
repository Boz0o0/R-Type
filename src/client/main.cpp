#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <span>

#include "client/ClientApp.hpp"
#include "client/ClientConfig.hpp"

namespace {

constexpr int32_t EXIT_ERROR = 84;

}

int32_t main(int32_t argc, char** argv) {
  try {
    const std::span args(argv, static_cast<std::size_t>(argc));
    const auto config = rtype::client::parseArguments(args);
    if (!config) {
      std::fprintf(stderr, "USAGE: %s [host] [port]\n", args.front());
      return EXIT_ERROR;
    }
    rtype::client::ClientApp app(*config);
    app.run();
  } catch (const std::exception& e) {
    std::fprintf(stderr, "[client] error: %s\n", e.what());
    return EXIT_ERROR;
  } catch (...) {
    std::fprintf(stderr, "[client] unknown error\n");
    return EXIT_ERROR;
  }
  return 0;
}
