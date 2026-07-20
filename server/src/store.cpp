#include "attokv_server/store.h"

using namespace attokv;

KVStore::KVStore() {}

std::string_view KVStore::get(const std::string& key) {
    auto it = m_map.find(key);
    if (it == m_map.end())
        return {};
    else
        return it->second;
}

void KVStore::set(const std::string& key, const std::string& value) {
    m_map.insert_or_assign(key, value);
}

void KVStore::flush() {
    m_map.clear();
}
