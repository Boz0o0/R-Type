#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <rtype/common/protocol/BinaryWriter.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <string_view>

namespace rtype::protocol {

void BinaryWriter::writeFloat(float value) {
  this->write(std::bit_cast<std::uint32_t>(value));
}

void BinaryWriter::writeString(std::string_view value) {
  if (value.size() > std::numeric_limits<std::uint8_t>::max()) {
    throw ProtocolError("string longer than 255 bytes");
  }
  this->write(static_cast<std::uint8_t>(value.size()));
  for (const char character : value) {
    this->_buffer.push_back(static_cast<std::byte>(character));
  }
}

}  // namespace rtype::protocol
