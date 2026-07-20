#ifndef ATTOKV_SERVER_STORE_H
#define ATTOKV_SERVER_STORE_H

#include <string>
#include <unordered_map>

namespace attokv {

class KVStore {
public:
    KVStore();

    std::string_view get(const std::string& key);

    void set(const std::string& key, const std::string& val);

    void flush();

private:
    std::unordered_map<std::string, std::string> m_map{};
};

} // namespace attokv

#endif
