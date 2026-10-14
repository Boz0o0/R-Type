#pragma once

#include <cstdint>

namespace ampersand::render {

/**
 * @brief Represents an RGBA color value used for clear and draw operations.
 */
struct Color {
    std::uint8_t r{};
    std::uint8_t g{};
    std::uint8_t b{};
    std::uint8_t a{255};
};

}  // namespace ampersand::render