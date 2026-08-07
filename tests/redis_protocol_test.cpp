#include "attokv/socket.h"
#include "attokv_benchmark/redis_protocol.h"
#include <expected>
#include <iostream>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <thread>

using namespace attokv;
using namespace attokv::benchmark::redis;

namespace {

int failures{};

void check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        failures++;
    }
}

std::expected<Reply, std::string> parse(std::string response) {
    int sockets[2]{};
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == -1)
        return std::unexpected{"socketpair failed"};
    Socket reader{sockets[0]};
    Socket writer{sockets[1]};
    std::thread sender{[&] {
        std::size_t middle = response.size() / 2;
        ::send(writer.native_handle(), response.data(), middle, 0);
        ::send(writer.native_handle(), response.data() + middle, response.size() - middle, 0);
        writer.reset();
    }};
    auto result = read_reply(reader);
    sender.join();
    return result;
}

void test_encoding() {
    std::string_view arguments[]{"SET", "key", "value"};
    check(encode_request(arguments) == "*3\r\n$3\r\nSET\r\n$3\r\nkey\r\n$5\r\nvalue\r\n",
          "request uses RESP2 arrays and bulk strings");
}

void test_replies() {
    auto status = parse("+OK\r\n");
    check(status && status->type == ReplyType::string && status->text == "OK",
          "simple string is parsed");

    auto bulk = parse("$5\r\nvalue\r\n");
    check(bulk && bulk->type == ReplyType::bulk && bulk->text == "value",
          "fragmented bulk string is parsed");

    auto nil = parse("$-1\r\n");
    check(nil && nil->type == ReplyType::nil, "nil bulk string is parsed");

    auto integer = parse(":1\r\n");
    check(integer && integer->type == ReplyType::integer && integer->integer == 1,
          "integer is parsed");

    check(!parse("-ERR failure\r\n"), "Redis error becomes an error result");
    check(!parse("$3\r\nabcxx"), "malformed bulk ending is rejected");
    check(!parse("$16777216\r\n"), "oversized bulk reply is rejected");
    check(!parse("!3\r\nabc\r\n"), "unsupported reply type is rejected");
    check(!parse(""), "unexpected EOF is rejected");
}

} // namespace

int main() {
    test_encoding();
    test_replies();
    return failures == 0 ? 0 : 1;
}
