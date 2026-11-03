#include <rtype/server/PasswordHasher.hpp>
#include <monocypher.h>
#include <random>
#include <vector>

namespace rtype::server {

Salt randomSalt() {
    std::random_device rd;
    Salt salt{};
    for (auto& byte : salt) byte = static_cast<std::uint8_t>(rd());
    return salt;
}

Hash hashPassword(std::string_view password, const Salt& salt) {
  crypto_argon2_config config{
      .algorithm = CRYPTO_ARGON2_ID,
      .nb_blocks = 19 * 1024,
      .nb_passes = 2,
      .nb_lanes = 1,
  };
  crypto_argon2_inputs inputs{
      .pass = reinterpret_cast<const std::uint8_t*>(password.data()),
      .salt = salt.data(),
      .pass_size = static_cast<std::uint32_t>(password.size()),
      .salt_size = static_cast<std::uint32_t>(salt.size()),
  };
  std::vector<std::uint8_t> workArea(config.nb_blocks * 1024);
  Hash hash{};
  crypto_argon2(hash.data(), hash.size(), workArea.data(), config, inputs,
                crypto_argon2_no_extras);
  return hash;
}

bool verifyPassword(std::string_view password, const Salt& salt, const Hash& expected) {
  const Hash actual = hashPassword(password, salt);
  return crypto_verify32(actual.data(), expected.data()) == 0;
}

} // namepace stype::server