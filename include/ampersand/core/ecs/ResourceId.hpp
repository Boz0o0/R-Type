#pragma once

#include <cstddef>

namespace ampersand::core::ecs {

/**
 * @brief Allocates a new slot in the World's resource array.
 *
 * @return Id of the new resource type.
 */
std::size_t allocateResourceId();

/**
 * @brief Returns the id of resource type T (its slot in the resource array).
 *
 * Ids are counted separately from component ids, so resources never use one
 * of the kMaxComponents columns.
 *
 * @tparam T Resource type.
 * @return Id of T.
 */
template <typename T>
std::size_t resourceId() {
    static const std::size_t index = allocateResourceId();
    return index;
}

}  // namespace ampersand::core::ecs
