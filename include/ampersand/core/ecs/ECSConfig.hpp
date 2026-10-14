#pragma once

#include <cstddef>
#include <cstdint>

namespace ampersand::core::ecs {

/** @brief Maximum number of distinct component types. */
inline constexpr std::size_t kMaxComponents = 64;

/** @brief Default maximum number of entities alive at once in a World. */
inline constexpr std::uint32_t kDefaultMaxEntities = 10000;

}  // namespace ampersand::core::ecs
