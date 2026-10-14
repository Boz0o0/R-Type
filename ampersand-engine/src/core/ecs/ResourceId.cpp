#include <ampersand/core/ecs/ResourceId.hpp>
#include <atomic>
#include <cstddef>

namespace ampersand::core::ecs {

namespace {

/** @brief Next resource id. */
std::atomic<std::size_t> nextResourceId{0};

}  // namespace

std::size_t allocateResourceId() { return nextResourceId.fetch_add(1); }

}  // namespace ampersand::core::ecs
