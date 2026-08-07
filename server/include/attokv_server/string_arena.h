#ifndef ATTOKV_STRING_ARENA_H
#define ATTOKV_STRING_ARENA_H

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace attokv {

class StringArena {
public:
    struct StringRef {
        uint32_t block;
        uint32_t offset;
        uint32_t size;

        bool empty() const {
            return size == 0;
        }
    };

    class CompactedLayout {
    public:
        CompactedLayout(std::vector<uint32_t> block_offsets, size_t size)
            : m_block_offsets{block_offsets}, m_size{size} {}

        size_t size() const noexcept;

        /// Given `ref` is a reference to a StringRef which existed in the arena before calling
        /// compact_into(), returns the new offset of the string into the new compacted bytes.
        uint32_t offset_of(StringRef ref) const;

    private:
        friend class StringArena;

        std::vector<uint32_t> m_block_offsets;
        size_t m_size{};
    };

    explicit StringArena(size_t block_size = 64 * 1024) : m_block_size{block_size}, m_blocks{} {
        allocate_block(block_size);
    }

    StringRef store(std::string_view string) {
        return store(std::as_bytes(std::span<const char>{string.data(), string.size()}));
    }

    StringRef store(std::span<const std::byte> bytes);

    std::string_view view(StringRef ref) const;

    /// Remove a string from the arena.
    /// Returns false if the string is not part of the arena.
    /// If this function returns true, the caller should call `should_reallocate`.
    bool remove(StringRef string);

    /// Should be called after `remove`.
    /// If returns true, allocate a new arena and move all active strings to it.
    bool should_reallocate() const;

    /// Return the number of bytes used for strings.
    /// This is useful for `compact_into`.
    size_t used_size() const;

    /// Copy all string bytes into `dest`. Invariant: `dest.size() >= used_size()`.
    /// Use the returned `CompactedLayout` to map StringRef offsets into the new compacted block.
    CompactedLayout compact_into(std::span<std::byte> dest);

private:
    struct Block {
        std::unique_ptr<std::byte[]> data;
        size_t capacity;
        size_t back{0};
        size_t dead{0};

        explicit Block(size_t capacity)
            : data{std::make_unique<std::byte[]>(capacity)}, capacity{capacity} {}

        size_t remaining() const {
            return capacity - back;
        }
    };

    Block& current_block() {
        return m_blocks.back();
    }

    void allocate_block(size_t capacity) {
        m_blocks.emplace_back(capacity);
    }

    size_t m_block_size;
    size_t m_dead{0};
    std::vector<Block> m_blocks{};
};
} // namespace attokv

#endif
