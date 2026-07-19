#include "kvcli/store.h"

using namespace kvcli;

KVStore::KVStore() {}

std::string_view KVStore::get(const std::string& key) {
    auto it = _map.find(key);
    if (it == _map.end()) return {};
    else return it->second;
}

void KVStore::set(const std::string& key, const std::string& value) {
    _map.insert_or_assign(key, value);
}

void KVStore::flush() {
    _map.clear();
}
