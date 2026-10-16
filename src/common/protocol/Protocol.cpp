#include <rtype/common/protocol/Protocol.hpp>
#include <string>

namespace rtype::protocol {

ProtocolError::ProtocolError(const std::string& message)
    : std::runtime_error("Protocol error: " + message) {}

}  // namespace rtype::protocol
