#pragma once
#include <array>
#include <cstdint>
#include <string_view>

namespace rtype::server {

using Salt = std::array<std::uint8_t, 16>;
using Hash = std::array<std::uint8_t, 32>;

Salt randomSalt();
Hash hashPassword(std::string_view password, const Salt& salt);
bool verifyPassword(std::string_view password, const Salt& salt, Hash& expected);

} // namespace rtype::server


