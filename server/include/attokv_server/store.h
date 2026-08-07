#ifndef ATTOKV_SERVER_STORE_H
#define ATTOKV_SERVER_STORE_H

#include "attokv_server/map.h"
#include <optional>
#include <string_view>

namespace attokv {

class KVStore {
public:
    KVStore();

    std::optional<std::string_view> get(std::string_view key) const;

    void set(std::string_view key, std::string_view value);

    bool remove(std::string_view key);

    void flush();

private:
    FastMap m_map{};
};

} // namespace attokv

#endif
