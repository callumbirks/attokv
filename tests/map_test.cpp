#include "attokv_server/map.h"
#include "attokv_server/string_arena.h"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

using namespace attokv;

namespace {

int failures{};

void check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        failures++;
    }
}

std::uint64_t fnv_1a(std::string_view input) {
    std::uint64_t hash = 0xcbf29ce484222325;
    for (char byte : input) {
        hash ^= byte;
        hash *= 0x100000001b3;
    }
    return hash;
}

std::vector<std::string> keys_for_slot(std::size_t slot, std::size_t count) {
    std::vector<std::string> keys{};
    for (std::size_t i = 0; keys.size() < count; i++) {
        std::string key = "collision" + std::to_string(i);
        if (fnv_1a(key) % 8 == slot)
            keys.push_back(std::move(key));
    }
    return keys;
}

void test_empty_and_missing_values() {
    FastMap map{};
    check(!map.get("missing"), "missing key returns nullopt");
    map.set("empty", "");
    auto empty = map.get("empty");
    check(empty.has_value(), "stored empty value exists");
    check(empty && empty->empty(), "stored empty value remains empty");
}

void test_delete_and_reuse() {
    FastMap map{8};
    auto colliding = keys_for_slot(7, 2);
    map.set(colliding[0], "one");
    map.set(colliding[1], "two");
    check(map.get(colliding[1]) && *map.get(colliding[1]) == "two",
          "linear probing wraps around the table");
    check(map.remove(colliding[0]), "existing entry is removed");
    check(!map.get(colliding[0]), "removed entry is missing");
    check(map.get(colliding[1]) && *map.get(colliding[1]) == "two", "probe chain survives delete");

    for (int i = 0; i < 32; i++) {
        map.set("reused", "value");
        check(map.remove("reused"), "tombstone slot can be reused");
    }
}

void test_clear() {
    FastMap map{};
    for (int i = 0; i < 100; i++)
        map.set("key" + std::to_string(i), "value");
    map.clear();
    check(!map.get("key50"), "clear removes prior values");
    map.set("after", "clear");
    check(map.get("after") && *map.get("after") == "clear", "map is reusable after clear");
}

void test_string_arena_boundaries() {
    StringArena arena{8};
    auto boundary = arena.store("12345678");
    check(arena.view(boundary) == "12345678", "block-sized value is stored in full");
    check(arena.remove(boundary), "block-ending value can be removed");

    auto oversized = arena.store("abcdefghijkl");
    check(arena.view(oversized) == "abcdefghijkl", "oversized value is stored in full");

    std::vector<std::byte> compacted(arena.used_size());
    auto layout = arena.compact_into(compacted);
    check(layout.size() == compacted.size(), "compacted layout reports its size");
    check(layout.offset_of(oversized) + oversized.size <= compacted.size(),
          "compacted offset is within destination");
}

} // namespace

int main() {
    test_empty_and_missing_values();
    test_delete_and_reuse();
    test_clear();
    test_string_arena_boundaries();
    return failures == 0 ? 0 : 1;
}
