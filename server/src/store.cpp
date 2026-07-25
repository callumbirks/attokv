#include "attokv_server/store.h"
#include <string_view>

using namespace attokv;

KVStore::KVStore() {}

std::string_view KVStore::get(std::string_view key) {
    return m_map.get(key);
}

void KVStore::set(std::string key, std::string value) {
    m_map.set(key, value);
}

void KVStore::flush() {
    m_map.clear();
}
