#ifndef ATTOKV_SERVER_MAP_H
#define ATTOKV_SERVER_MAP_H

#include "attokv_server/string_arena.h"
#include <bitset>
#include <memory>
#include <string_view>

namespace attokv {

constexpr size_t k_default_capacity = 8;
constexpr double k_load_factor = 0.75;
constexpr size_t k_arena_block_size = 64 * 1024;

namespace fastmap {

namespace flags {
static constexpr size_t k_deleted = 0;
}

struct Entry {
    std::bitset<8> flags;
    StringArena::StringRef key;
    StringArena::StringRef value;

    bool empty() const {
        return key.empty();
    }

    bool deleted() const {
        return flags.test(flags::k_deleted);
    }

    void mark_deleted() {
        flags.set(flags::k_deleted);
    }

    void unmark_deleted() {
        flags.reset(flags::k_deleted);
    }
};

} // namespace fastmap

class FastMap {
public:
    FastMap() : FastMap(k_default_capacity) {}
    explicit FastMap(size_t capacity)
        : m_capacity{capacity}, m_arena{k_arena_block_size},
          m_table{std::make_unique<attokv::fastmap::Entry[]>(capacity)} {}

    std::string_view get(std::string_view key) const;

    void set(std::string_view key, std::string_view value);

    bool remove(std::string_view key);

    void clear();

private:
    size_t m_capacity{0};
    size_t m_size{0};
    StringArena m_arena{};
    std::unique_ptr<attokv::fastmap::Entry[]> m_table{};
};

} // namespace attokv

#endif
