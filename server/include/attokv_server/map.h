#ifndef ATTOKV_SERVER_MAP_H
#define ATTOKV_SERVER_MAP_H

#include "attokv_server/string_arena.h"
#include <memory>
#include <string_view>

namespace attokv {

constexpr size_t k_default_capacity = 8;
constexpr double k_load_factor = 0.75;
constexpr size_t k_arena_block_size = 64 * 1024;

namespace fastmap {

struct Entry {
    StringArena::StringRef key;
    StringArena::StringRef value;

    bool empty() {
        return key.empty();
    }
};

} // namespace fastmap

using namespace attokv::fastmap;

class FastMap {
public:
    FastMap() : FastMap(k_default_capacity) {}
    explicit FastMap(size_t capacity)
        : m_capacity{capacity}, m_arena{k_arena_block_size},
          m_table{std::make_unique<Entry[]>(capacity)} {}

    std::string_view get(std::string_view key);

    void set(std::string_view key, std::string_view value);

    bool remove(std::string_view key);

    void clear();

private:
    size_t m_capacity{0};
    size_t m_size{0};
    StringArena m_arena{};
    std::unique_ptr<Entry[]> m_table{};
};

} // namespace attokv

#endif
