#ifndef ATTOKV_SERVER_STORE_H
#define ATTOKV_SERVER_STORE_H

#include "attokv_server/map.h"
#include <string>
#include <string_view>

namespace attokv {

class KVStore {
public:
    KVStore();

    std::string_view get(std::string_view key);

    void set(std::string key, std::string value);

    void flush();

private:
    FastMap m_map{};
};

} // namespace attokv

#endif
