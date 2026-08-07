#include "attokv_server/map.h"
#include "attokv_server/string_arena.h"
#include <algorithm>
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
Entry* get_entry(const std::unique_ptr<Entry[]>& table, size_t capacity, const StringArena& arena,
                 std::string_view key) {
    uint64_t hash = fnv_1a(key);
    size_t slot = hash % capacity;
    Entry* deleted_entry{};
    for (size_t checked = 0; checked < capacity; checked++) {
        Entry& entry = table[slot];
        if (entry.empty() && !entry.deleted()) {
            if constexpr (SLOT_MATCH == SlotMatch::empty ||
                          SLOT_MATCH == SlotMatch::match_or_empty) {
                return deleted_entry ? deleted_entry : &entry;
            }
            return nullptr;
        }

        if (entry.deleted()) {
            if constexpr (SLOT_MATCH == SlotMatch::match_or_empty) {
                if (!deleted_entry)
                    deleted_entry = &entry;
            }
            slot = (slot + 1) % capacity;
            continue;
        }

        if constexpr (SLOT_MATCH == SlotMatch::match || SLOT_MATCH == SlotMatch::match_or_empty) {
            if (arena.view(entry.key) == key) {
                return &entry;
            }
        }
        slot = (slot + 1) % capacity;
    }

    if constexpr (SLOT_MATCH == SlotMatch::empty || SLOT_MATCH == SlotMatch::match_or_empty) {
        return deleted_entry;
    }
    return nullptr;
}

std::unique_ptr<Entry[]> resize(std::unique_ptr<Entry[]>& table, size_t old_capacity,
                                const StringArena& arena, size_t new_capacity) {
    std::unique_ptr<Entry[]> new_table = std::make_unique<Entry[]>(new_capacity);
    // Rehash all entries into the new table
    for (size_t i = 0; i < old_capacity; i++) {
        if (!table[i].empty()) {
            // There should be no matches, so unlike set(), we only look for empty slots
            Entry* new_entry = get_entry<SlotMatch::empty>(new_table, new_capacity, arena,
                                                           arena.view(table[i].key));
            new_entry->key = table[i].key;
            new_entry->value = table[i].value;
        }
    }
    return new_table;
}

std::optional<std::string_view> FastMap::get(std::string_view key) const {
    Entry* entry = get_entry<SlotMatch::match>(m_table, m_capacity, m_arena, key);
    if (!entry)
        return std::nullopt;
    return m_arena.view(entry->value);
}

void FastMap::set(std::string_view key, std::string_view value) {
    Entry* entry = get_entry<SlotMatch::match_or_empty>(m_table, m_capacity, m_arena, key);
    if (entry->empty()) {
        if (m_size >= m_capacity * k_load_factor) {
            m_table = resize(m_table, m_capacity, m_arena, m_capacity * 2);
            m_capacity *= 2;
            entry = get_entry<SlotMatch::match_or_empty>(m_table, m_capacity, m_arena, key);
        }
        entry->unmark_deleted();
        entry->key = m_arena.store(key);
        m_size++;
    }
    entry->value = m_arena.store(value);
}

bool FastMap::remove(std::string_view key) {
    Entry* entry = get_entry<SlotMatch::match>(m_table, m_capacity, m_arena, key);
    if (!entry)
        return false;
    m_arena.remove(entry->key);
    m_arena.remove(entry->value);
    *entry = {};
    entry->mark_deleted();
    m_size--;

    if (m_arena.should_reallocate()) {
        StringArena new_arena{k_arena_block_size};
        for (size_t i = 0; i < m_capacity; i++) {
            Entry* e = &m_table[i];
            if (e->empty())
                continue;
            e->key = new_arena.store(m_arena.view(e->key));
            e->value = new_arena.store(m_arena.view(e->value));
        }
        m_arena = std::move(new_arena);
    }

    return true;
}

void FastMap::clear() {
    std::fill_n(m_table.get(), m_capacity, Entry{});
    m_arena = StringArena{k_arena_block_size};
    m_size = 0;
}
