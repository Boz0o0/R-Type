#include <gtest/gtest.h>

#include <filesystem>

namespace {

// Every Game reads game/ from the working directory, like the server run from
// the repository.
class GameFolder final : public testing::Environment {
 public:
  void SetUp() override { std::filesystem::current_path(RTYPE_SOURCE_DIR); }
};

const testing::Environment* const gameFolder =
    testing::AddGlobalTestEnvironment(new GameFolder);

}  // namespace
