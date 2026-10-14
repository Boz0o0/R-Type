#pragma once

#include <cstddef>

namespace ampersand::core::ecs {

/**
 * @brief Allocates a new column in the position table.
 *
 * @return Column of the new component type.
 */
std::size_t allocateComponentId();

/**
 * @brief Returns the column of component type T in the position table.
 *
 * @tparam T Component type.
 * @return Column of T.
 */
template <typename T>
std::size_t componentId() {
    static const std::size_t column = allocateComponentId();
    return column;
}

}  // namespace ampersand::core::ecs
