#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <type_traits>

namespace ampersand::net {

/**
 * @brief Reads big-endian values from a byte buffer, with bounds checks.
 *
 * Every read returns false instead of going past the end of the buffer, and
 * leaves the output untouched in that case. A reader never throws and never
 * reads out of bounds, whatever bytes it is given, so it can be used directly
 * on datagrams received from the network.
 */
class BinaryReader {
public:
    /**
     * @brief Reads from a buffer.
     *
     * @param data Bytes to read; must outlive the reader.
     */
    explicit BinaryReader(std::span<const std::byte> data) noexcept
        : _data(data) {}

    /** @brief Reads an unsigned integer, most significant byte first. */
    template <std::unsigned_integral T>
    [[nodiscard]] bool read(T& out) noexcept {
        if (this->remaining() < sizeof(T)) {
            return false;
        }
        std::uint64_t value = 0;
        for (std::size_t index = 0; index < sizeof(T); ++index) {
            value = (value << 8U) | std::to_integer<std::uint64_t>(
                                        this->_data[this->_offset + index]);
        }
        this->_offset += sizeof(T);
        out = static_cast<T>(value);
        return true;
    }

    /** @brief Reads a float; NaN and infinities are rejected. */
    [[nodiscard]] bool readFloat(float& out) noexcept;

    /**
     * @brief Reads an enum and checks it is not above its last value.
     *
     * @param out Filled with the value read.
     * @param last Highest valid value of the enum (values start at 0).
     */
    template <typename E>
        requires std::is_enum_v<E>
    [[nodiscard]] bool readEnum(E& out, E last) noexcept {
        std::underlying_type_t<E> raw = 0;
        if (!this->read(raw) ||
            raw > static_cast<std::underlying_type_t<E>>(last)) {
            return false;
        }
        out = static_cast<E>(raw);
        return true;
    }

    /**
     * @brief Reads a string written by BinaryWriter::writeString.
     *
     * @param out Filled with the string read.
     * @param maxLength Longest length accepted, in bytes.
     */
    [[nodiscard]] bool readString(std::string& out, std::size_t maxLength);

    /** @brief Number of bytes not read yet. */
    [[nodiscard]] std::size_t remaining() const noexcept {
        return this->_data.size() - this->_offset;
    }

    /** @brief Whether every byte has been read. */
    [[nodiscard]] bool atEnd() const noexcept { return this->remaining() == 0; }

private:
    std::span<const std::byte> _data;
    std::size_t _offset{0};
};

}  // namespace ampersand::net
