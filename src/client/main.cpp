#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <rtype/client/ClientApp.hpp>
#include <rtype/client/ClientConfig.hpp>
#include <span>

int32_t main(int32_t argc, char** argv) {
  using rtype::client::ClientApp;
  using rtype::client::ClientConfig;

  try {
    const std::span args(argv, static_cast<std::size_t>(argc));
    const auto config = ClientConfig::fromArguments(args);
    if (!config) {
      std::fprintf(stderr, "USAGE: %s [host] [port] [name]\n", args.front());
      return ClientApp::EXIT_ERROR;
    }
    ClientApp app(*config);
    app.run();
  } catch (const std::exception& e) {
    std::fprintf(stderr, "[client] error: %s\n", e.what());
    return ClientApp::EXIT_ERROR;
  } catch (...) {
    std::fprintf(stderr, "[client] unknown error\n");
    return ClientApp::EXIT_ERROR;
  }
  return 0;
}
