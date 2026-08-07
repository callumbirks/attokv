#include "attokv_server/string_arena.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

using namespace attokv;

size_t StringArena::CompactedLayout::size() const noexcept {
    return m_size;
}

uint32_t StringArena::CompactedLayout::offset_of(StringRef ref) const {
    assert(ref.block < m_block_offsets.size());
    return m_block_offsets[ref.block] + ref.offset;
}

StringArena::StringRef StringArena::store(std::span<const std::byte> bytes) {
    if (bytes.empty())
        return {};
    if (m_blocks.empty() || current_block().remaining() < bytes.size()) {
        allocate_block(std::max(m_block_size, bytes.size()));
    }
    Block& block = current_block();

    size_t offset = block.back;
    size_t size = bytes.size();

    std::memcpy(block.data.get() + offset, bytes.data(), size);

    block.back += size;

    return {
        static_cast<uint32_t>(m_blocks.size() - 1), static_cast<uint32_t>(offset),
        static_cast<uint32_t>(size)
    };
}

StringArena::StringRef StringArena::overwrite(StringRef ref, std::string_view string) {
    assert(string.size() <= ref.size);
    if (ref.empty())
        return {};
    if (ref.block >= m_blocks.size())
        return {};
    const Block& block = m_blocks[ref.block];
    if (ref.offset + ref.size > block.back)
        return {};
    std::memcpy(block.data.get() + ref.offset, string.data(), string.size());
    size_t difference = string.size() - ref.size;
    if (difference > 0) {
        std::memset(block.data.get() + ref.offset + string.size(), 0, difference);
    }
    return {.block = ref.block, .offset = ref.offset, .size = static_cast<uint32_t>(string.size())};
}

bool StringArena::remove(StringArena::StringRef string) {
    if (string.block >= m_blocks.size())
        return false;

    Block& block = m_blocks[string.block];

    if (string.offset >= block.back || string.size > block.back - string.offset) {
        return false;
    }

    std::memset(block.data.get() + string.offset, 0, string.size);

    block.dead += string.size;
    m_dead += string.size;
    return true;
}

bool StringArena::should_reallocate() const {
    size_t capacity{};
    for (const Block& block : m_blocks)
        capacity += block.capacity;
    return capacity <= m_dead * 2;
}

size_t StringArena::used_size() const {
    size_t capacity{0};
    for (const Block& block : m_blocks) {
        capacity += block.back;
    }
    return capacity;
}

StringArena::CompactedLayout StringArena::compact_into(std::span<std::byte> dest) {
    [[maybe_unused]]
    size_t capacity = used_size();
    assert(dest.size() == capacity);

    std::vector<uint32_t> block_offsets(m_blocks.size());
    {
        size_t processed{0};
        for (size_t i = 0; i < m_blocks.size(); i++) {
            Block& block = m_blocks[i];
            std::memcpy(dest.data() + processed, block.data.get(), block.back);
            block_offsets[i] = processed;
            processed += block.back;
        }
    }

    return {std::move(block_offsets), dest.size()};
}

std::string_view StringArena::view(StringArena::StringRef ref) const {
    if (ref.empty())
        return {};
    if (ref.block >= m_blocks.size())
        return {};
    const Block& block = m_blocks[ref.block];
    if (ref.offset + ref.size > block.back)
        return {};
    return {reinterpret_cast<const char*>(block.data.get() + ref.offset), ref.size};
}
