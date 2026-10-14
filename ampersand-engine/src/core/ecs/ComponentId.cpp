#include <ampersand/core/ecs/ComponentId.hpp>
#include <ampersand/core/ecs/ECSConfig.hpp>
#include <ampersand/core/ecs/ECSExceptions.hpp>
#include <atomic>
#include <cstddef>

namespace ampersand::core::ecs {

namespace {

/** @brief Next column to hand out. (next component id) */
std::atomic<std::size_t> nextColumn{0};

}  // namespace

std::size_t allocateComponentId() {
    const std::size_t column = nextColumn.fetch_add(1);
    if (column >= kMaxComponents) {
        throw ComponentLimitReached(std::to_string(kMaxComponents) +
                                    " component types");
    }
    return column;
}

}  // namespace ampersand::core::ecs
