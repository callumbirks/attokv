#include "attokv_server/map.h"
#include "attokv_server/string_arena.h"
#include <cstdint>
#include <cstring>
#include <memory>
#include <string_view>

using namespace attokv;
using namespace attokv::fastmap;

constexpr uint64_t k_fnv_offset = 0xcbf29ce484222325;
constexpr uint64_t k_fnv_prime = 0x100000001b3;

uint64_t fnv_1a(std::string_view input) {
    uint64_t hash = k_fnv_offset;

    for (char b : input) {
        hash ^= b;
        hash *= k_fnv_prime;
    }

    return hash;
}

size_t get_slot(std::string_view key, size_t capacity) {
    uint64_t hash = fnv_1a(key);
    return hash % capacity;
}

enum class SlotMatch {
    empty,
    match_or_empty,
    match,
};

template <SlotMatch SLOT_MATCH>
Entry* get_entry(std::unique_ptr<Entry[]>& table, size_t capacity, std::string_view key) {
    size_t slot = get_slot(key, capacity);
    while (!table[slot].empty()) {
        if constexpr (SLOT_MATCH == SlotMatch::match || SLOT_MATCH == SlotMatch::match_or_empty) {
            Entry& entry = table[slot];
            if (entry.key == key) {
                return &entry;
            }
        }
        slot += 1;
    }
    if constexpr (SLOT_MATCH == SlotMatch::empty || SLOT_MATCH == SlotMatch::match_or_empty) {
        return &table[slot];
    }
    return nullptr;
}

std::unique_ptr<Entry[]> resize(std::unique_ptr<Entry[]>& table, size_t old_capacity,
                                size_t new_capacity) {
    std::unique_ptr<Entry[]> new_table = std::make_unique<Entry[]>(new_capacity);
    // Rehash all entries into the new table
    for (size_t i = 0; i < old_capacity; i++) {
        if (!table[i].empty()) {
            // There should be no matches, so unlike set(), we only look for empty slots
            Entry* new_entry = get_entry<SlotMatch::empty>(new_table, new_capacity, table[i].key);
            new_entry->key = std::move(table[i].key);
            new_entry->value = std::move(table[i].value);
        }
    }
    return new_table;
}

std::string_view FastMap::get(std::string_view key) {
    Entry* entry = get_entry<SlotMatch::match>(m_table, m_capacity, key);
    if (!entry)
        return {};
    return entry->value;
}

void FastMap::set(std::string_view key, std::string_view value) {
    Entry* entry = get_entry<SlotMatch::match_or_empty>(m_table, m_capacity, key);
    if (entry->empty()) {
        if (m_size >= m_capacity * k_load_factor) {
            m_table = resize(m_table, m_capacity, m_capacity * 2);
            m_capacity *= 2;
            entry = get_entry<SlotMatch::match_or_empty>(m_table, m_capacity, key);
        }
        entry->key = m_arena.store(key);
        m_size++;
    }
    entry->value = m_arena.store(value);
}

void FastMap::clear() {
    std::memset((void*)m_table.get(), 0, m_capacity);
    m_arena = StringArena{k_arena_block_size};
}
