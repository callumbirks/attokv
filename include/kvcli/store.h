#ifndef KVCLI_STORE_H
#define KVCLI_STORE_H

#include <string>
#include <unordered_map>

namespace kvcli {

class KVStore {
public:
    KVStore();

    std::string_view get(const std::string& key);

    void set(const std::string& key, const std::string& val);

    void flush();
private:
    std::unordered_map<std::string, std::string> _map{};
};

}

#endif
