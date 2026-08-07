#ifndef ATTOKV_BENCHMARK_REDIS_PROTOCOL_H
#define ATTOKV_BENCHMARK_REDIS_PROTOCOL_H

#include "attokv/socket.h"
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <string_view>

namespace attokv::benchmark::redis {

enum class ReplyType {
    string,
    bulk,
    nil,
    integer,
};

struct Reply {
    ReplyType type;
    std::string text{};
    std::int64_t integer{};
};

std::string encode_request(std::span<const std::string_view> arguments);
std::expected<Reply, std::string> read_reply(const Socket& socket);

} // namespace attokv::benchmark::redis

#endif
