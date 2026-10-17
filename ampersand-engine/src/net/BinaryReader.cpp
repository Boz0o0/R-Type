#include <ampersand/net/BinaryReader.hpp>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>

namespace ampersand::net {

bool BinaryReader::readFloat(float& out) noexcept {
    std::uint32_t bits = 0;
    if (!this->read(bits)) {
        return false;
    }
    const auto value = std::bit_cast<float>(bits);
    if (!std::isfinite(value)) {
        return false;
    }
    out = value;
    return true;
}

bool BinaryReader::readString(std::string& out, std::size_t maxLength) {
    std::uint8_t length = 0;
    if (!this->read(length) || length > maxLength ||
        length > this->remaining()) {
        return false;
    }
    out.assign(
        reinterpret_cast<const char*>(this->_data.data() + this->_offset),
        length);
    this->_offset += length;
    return true;
}

}  // namespace ampersand::net
