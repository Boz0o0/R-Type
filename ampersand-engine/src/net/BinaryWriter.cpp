#include <ampersand/net/BinaryWriter.hpp>
#include <ampersand/net/UdpSocket.hpp>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>

namespace ampersand::net {

void BinaryWriter::writeFloat(float value) {
    this->write(std::bit_cast<std::uint32_t>(value));
}

void BinaryWriter::writeString(std::string_view value) {
    if (value.size() > std::numeric_limits<std::uint8_t>::max()) {
        throw NetError("string longer than 255 bytes");
    }
    this->write(static_cast<std::uint8_t>(value.size()));
    for (const char character : value) {
        this->_buffer.push_back(static_cast<std::byte>(character));
    }
}

}  // namespace ampersand::net
