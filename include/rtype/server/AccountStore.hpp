#pragma once
#include <cstdint>
#include <optional>
#include <string>

struct sqlite3;

namespace rtype::server {

class AccountStore {
    public:
        explicit AccountStore(const std::string& path);
        ~AccountStore();
        AccountStore(const AccountStore&) = delete;
        AccountStore& operator=(const AccountStore&) = delete;

        bool create(const std::string& username, const std::string& password);

        std::optional<std::int64_t> authenticatedsqs(const std::string& username,
                                           const std::string& password);
    private:
        sqlite3* _db{};
};

} // namespace rtype::server