#include "attokv_server/store.h"
#include <string_view>

using namespace attokv;

KVStore::KVStore() {}

std::string_view KVStore::get(std::string_view key) {
    return m_map.get(key);
}

void KVStore::set(std::string_view key, std::string_view value) {
    m_map.set(key, value);
}

bool KVStore::remove(std::string_view key) {
    return m_map.remove(key);
}

void KVStore::flush() {
    m_map.clear();
}
