#ifndef ATTOKV_SERVER_STRING_ARENA_H
#define ATTOKV_SERVER_STRING_ARENA_H

#include <cstdint>
#include <cstring>
#include <memory>
#include <string_view>
#include <vector>

namespace attokv {

class StringArena {
public:
    struct StringRef {
        const char* data;
        uint32_t block;
        uint32_t size;

        bool empty() {
            return size == 0;
        }

        operator std::string_view() {
            return {data, static_cast<size_t>(size)};
        }
    };

    explicit StringArena(size_t block_size = 64 * 1024) : m_block_size{block_size}, m_blocks{} {
        allocate_block();
    }

    StringRef store(std::string_view string) {
        if (string.empty())
            return {};
        if (m_blocks.empty() || current_block().remaining() < string.size()) {
            allocate_block();
        }
        Block& block = current_block();
        char* start = block.data.get() + block.back;
        std::memcpy(start, string.data(), string.size());
        block.back += string.size();
        return {
            start, static_cast<uint32_t>(m_blocks.size() - 1), static_cast<uint32_t>(string.size())
        };
    }

    /// Remove a string from the arena.
    /// Returns false if the string is not part of the arena.
    /// If this function returns true, the caller should call `should_reallocate`.
    bool remove(StringRef string) {
        if (string.block >= m_blocks.size())
            return false;

        Block& block = m_blocks[string.block];

        if (string.data < block.data.get() || string.data >= block.data.get() + block.capacity) {
            return false;
        }

        std::memset(const_cast<char*>(string.data), 0, string.size);

        block.dead += string.size;
        m_dead += string.size;
        return true;
    }

    /// Should be called after `remove`.
    /// If returns true, allocate a new arena and move all active strings to it.
    bool should_reallocate() {
        return m_blocks.size() * m_block_size <= m_dead * 2;
    }

private:
    struct Block {
        std::unique_ptr<char[]> data;
        size_t capacity;
        size_t back{0};
        size_t dead{0};

        explicit Block(size_t capacity)
            : data{std::make_unique<char[]>(capacity)}, capacity{capacity} {}

        size_t remaining() {
            return capacity - back;
        }
    };

    Block& current_block() {
        return m_blocks.back();
    }

    void allocate_block() {
        m_blocks.emplace_back(m_block_size);
    }

    size_t m_block_size;
    size_t m_dead{0};
    std::vector<Block> m_blocks{};
};
} // namespace attokv

#endif
