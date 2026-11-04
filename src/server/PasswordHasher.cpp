#include <monocypher.h>

#include <cstdint>
#include <random>
#include <rtype/server/PasswordHasher.hpp>
#include <string_view>
#include <vector>

namespace rtype::server {

namespace {

/** @brief Memory used by one hash, in KiB (OWASP: 19 MiB, 2 passes). */
constexpr std::uint32_t ARGON2_BLOCKS = 19U * 1024U;
constexpr std::uint32_t ARGON2_PASSES = 2;
constexpr std::size_t ARGON2_BLOCK_SIZE = 1024;

}  // namespace

Salt randomSalt() {
  // Backed by the random source of the OS on Linux, macOS and Windows.
  std::random_device device;
  Salt salt{};
  for (auto& byte : salt) {
    byte = static_cast<std::uint8_t>(device());
  }
  return salt;
}

Hash hashPassword(std::string_view password, const Salt& salt) {
  const crypto_argon2_config config{
      .algorithm = CRYPTO_ARGON2_ID,
      .nb_blocks = ARGON2_BLOCKS,
      .nb_passes = ARGON2_PASSES,
      .nb_lanes = 1,
  };
  const crypto_argon2_inputs inputs{
      .pass = reinterpret_cast<const std::uint8_t*>(password.data()),
      .salt = salt.data(),
      .pass_size = static_cast<std::uint32_t>(password.size()),
      .salt_size = static_cast<std::uint32_t>(salt.size()),
  };
  std::vector<std::uint8_t> workArea(ARGON2_BLOCKS * ARGON2_BLOCK_SIZE);
  Hash hash{};
  crypto_argon2(hash.data(), static_cast<std::uint32_t>(hash.size()),
                workArea.data(), config, inputs, crypto_argon2_no_extras);
  return hash;
}

bool verifyPassword(std::string_view password, const Salt& salt,
                    const Hash& expected) {
  const Hash actual = hashPassword(password, salt);
  return crypto_verify32(actual.data(), expected.data()) == 0;
}

}  // namespace rtype::server
