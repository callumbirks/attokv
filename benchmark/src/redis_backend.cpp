#include "attokv/socket.h"
#include "attokv/util.h"
#include "attokv_benchmark/backend.h"
#include "attokv_benchmark/redis_protocol.h"
#include <cerrno>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <expected>
#include <format>
#include <memory>
#include <netinet/in.h>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <sys/socket.h>

namespace attokv::benchmark {

std::unique_ptr<Backend> make_attokv_backend();

namespace {

constexpr std::size_t k_max_reply_size = 16'777'215;

std::string system_error(std::string_view context) {
    return std::format("{}: {}", context, std::strerror(errno));
}

std::expected<void, std::string> write_all(const Socket& socket, std::string_view input) {
    std::size_t written{};
    while (written < input.size()) {
        ssize_t result = ::send(socket.native_handle(), input.data() + written,
                                input.size() - written, MSG_NOSIGNAL);
        if (result > 0) {
            written += static_cast<std::size_t>(result);
            continue;
        }
        if (result == -1 && errno == EINTR)
            continue;
        return std::unexpected{system_error("Failed to write Redis request")};
    }
    return {};
}

std::expected<void, std::string> read_exact(const Socket& socket, std::span<char> output) {
    std::size_t read{};
    while (read < output.size()) {
        ssize_t result =
              ::recv(socket.native_handle(), output.data() + read, output.size() - read, 0);
        if (result > 0) {
            read += static_cast<std::size_t>(result);
            continue;
        }
        if (result == 0)
            return std::unexpected{"Redis closed the connection"};
        if (errno == EINTR)
            continue;
        return std::unexpected{system_error("Failed to read Redis response")};
    }
    return {};
}

std::expected<std::string, std::string> read_line(const Socket& socket) {
    std::string line{};
    while (line.size() <= k_max_reply_size) {
        char byte{};
        auto result = read_exact(socket, std::span<char>{&byte, 1});
        if (!result)
            return std::unexpected{std::move(result.error())};
        if (byte == '\r') {
            char newline{};
            result = read_exact(socket, std::span<char>{&newline, 1});
            if (!result)
                return std::unexpected{std::move(result.error())};
            if (newline != '\n')
                return std::unexpected{"Malformed Redis line ending"};
            return line;
        }
        line.push_back(byte);
    }
    return std::unexpected{"Redis response line is too large"};
}

std::expected<std::int64_t, std::string> parse_integer(std::string_view input) {
    std::int64_t number{};
    const char* end = input.data() + input.size();
    auto result = std::from_chars(input.data(), end, number);
    if (result.ec != std::errc{} || result.ptr != end)
        return std::unexpected{std::format("Invalid Redis integer: {}", input)};
    return number;
}

} // namespace

std::expected<redis::Reply, std::string> redis::read_reply(const Socket& socket) {
    char prefix{};
    auto prefix_result = read_exact(socket, std::span<char>{&prefix, 1});
    if (!prefix_result)
        return std::unexpected{std::move(prefix_result.error())};

    auto line = read_line(socket);
    if (!line)
        return std::unexpected{std::move(line.error())};

    if (prefix == '-')
        return std::unexpected{std::format("Redis error: {}", *line)};
    if (prefix == '+')
        return Reply{.type = ReplyType::string, .text = std::move(*line)};
    if (prefix == ':') {
        auto number = parse_integer(*line);
        if (!number)
            return std::unexpected{std::move(number.error())};
        return Reply{.type = ReplyType::integer, .integer = *number};
    }
    if (prefix != '$')
        return std::unexpected{std::format("Unsupported Redis reply type: {}", prefix)};

    auto size = parse_integer(*line);
    if (!size)
        return std::unexpected{std::move(size.error())};
    if (*size == -1)
        return Reply{.type = ReplyType::nil};
    if (*size < 0 || static_cast<std::uint64_t>(*size) > k_max_reply_size)
        return std::unexpected{"Invalid Redis bulk string size"};

    Reply reply{
        .type = ReplyType::bulk, .text = std::string(static_cast<std::size_t>(*size), '\0')
    };
    auto payload_result = read_exact(socket, reply.text);
    if (!payload_result)
        return std::unexpected{std::move(payload_result.error())};
    char ending[2]{};
    auto ending_result = read_exact(socket, ending);
    if (!ending_result)
        return std::unexpected{std::move(ending_result.error())};
    if (ending[0] != '\r' || ending[1] != '\n')
        return std::unexpected{"Malformed Redis bulk string ending"};
    return reply;
}

std::string redis::encode_request(std::span<const std::string_view> arguments) {
    std::string output = std::format("*{}\r\n", arguments.size());
    for (std::string_view argument : arguments) {
        output += std::format("${}\r\n", argument.size());
        output.append(argument);
        output += "\r\n";
    }
    return output;
}

namespace {

using redis::Reply;
using redis::ReplyType;

class RedisBackend final : public Backend {
public:
    std::expected<void, std::string> connect(const std::string& host, int port) override {
        Socket socket{::socket(AF_INET, SOCK_STREAM, 0)};
        if (!socket.is_valid())
            return std::unexpected{system_error("Failed to create Redis socket")};
        auto address = util::make_sockaddr(host, port);
        if (!address)
            return std::unexpected{address.error()};
        if (::connect(socket.native_handle(), reinterpret_cast<sockaddr*>(&*address),
                      sizeof(*address)) == -1) {
            return std::unexpected{system_error("Failed to connect to Redis")};
        }
        auto nodelay_result = socket.set_nodelay();
        if (!nodelay_result)
            return std::unexpected{nodelay_result.error().message()};
        m_socket = std::move(socket);
        return {};
    }

    std::expected<void, std::string> ping() override {
        auto reply = command({"PING"});
        if (!reply)
            return std::unexpected{std::move(reply.error())};
        if (reply->type != ReplyType::string || reply->text != "PONG")
            return std::unexpected{"Unexpected Redis PING response"};
        return {};
    }

    std::expected<void, std::string> flush() override {
        return expect_ok(command({"FLUSHDB"}));
    }

    std::expected<void, std::string> set(std::string_view key, std::string_view value) override {
        return expect_ok(command({"SET", key, value}));
    }

    std::expected<std::optional<std::string>, std::string> get(std::string_view key) override {
        auto reply = command({"GET", key});
        if (!reply)
            return std::unexpected{std::move(reply.error())};
        if (reply->type == ReplyType::nil)
            return std::optional<std::string>{};
        if (reply->type != ReplyType::bulk)
            return std::unexpected{"Unexpected Redis GET response"};
        return std::optional<std::string>{std::move(reply->text)};
    }

    std::expected<bool, std::string> remove(std::string_view key) override {
        auto reply = command({"DEL", key});
        if (!reply)
            return std::unexpected{std::move(reply.error())};
        if (reply->type != ReplyType::integer || (reply->integer != 0 && reply->integer != 1))
            return std::unexpected{"Unexpected Redis DEL response"};
        return reply->integer == 1;
    }

private:
    std::expected<Reply, std::string> command(std::initializer_list<std::string_view> arguments) {
        std::string request{redis::encode_request(arguments)};
        auto write_result = write_all(m_socket, request);
        if (!write_result)
            return std::unexpected{std::move(write_result.error())};
        return redis::read_reply(m_socket);
    }

    std::expected<void, std::string> expect_ok(std::expected<Reply, std::string> reply) {
        if (!reply)
            return std::unexpected{std::move(reply.error())};
        if (reply->type != ReplyType::string || reply->text != "OK")
            return std::unexpected{"Unexpected Redis status response"};
        return {};
    }

    Socket m_socket{};
};

} // namespace

std::unique_ptr<Backend> make_backend(BackendKind kind) {
    if (kind == BackendKind::attokv)
        return make_attokv_backend();
    return std::make_unique<RedisBackend>();
}

} // namespace attokv::benchmark
