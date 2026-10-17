#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>
#include <type_traits>
#include <vector>

namespace ampersand::net {

static_assert(std::numeric_limits<float>::is_iec559,
              "BinaryWriter sends floats as IEEE 754 binary32");

/**
 * @brief Appends values to a byte buffer in network byte order (big-endian).
 *
 * Values are written one by one, so the bytes are the same whatever the
 * compiler, the padding of structs or the endianness of the machine.
 */
class BinaryWriter {
public:
    /**
     * @brief Writes at the end of a buffer.
     *
     * @param buffer Buffer to append to; must outlive the writer.
     */
    explicit BinaryWriter(std::vector<std::byte>& buffer) : _buffer(buffer) {}

    /** @brief Appends an unsigned integer, most significant byte first. */
    template <std::unsigned_integral T>
    void write(T value) {
        const auto wide = static_cast<std::uint64_t>(value);
        for (std::size_t shift = sizeof(T) * 8; shift > 0; shift -= 8) {
            this->_buffer.push_back(
                static_cast<std::byte>(wide >> (shift - 8)));
        }
    }

    /** @brief Appends a float as its IEEE 754 bits. */
    void writeFloat(float value);

    /** @brief Appends an enum as its underlying integer. */
    template <typename E>
        requires std::is_enum_v<E>
    void writeEnum(E value) {
        this->write(static_cast<std::underlying_type_t<E>>(value));
    }

    /**
     * @brief Appends a string as a 1-byte length followed by its bytes.
     *
     * @throws NetError If the string is longer than 255 bytes.
     */
    void writeString(std::string_view value);

    /** @brief Total size of the buffer. */
    [[nodiscard]] std::size_t size() const noexcept {
        return this->_buffer.size();
    }

private:
    std::vector<std::byte>& _buffer;
};

}  // namespace ampersand::net
