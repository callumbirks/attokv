#include "attokv_client/client.h"
#include "attokv/message.h"
#include "attokv/util.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <err.h>
#include <expected>
#include <format>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace attokv;

std::expected<void, std::string> Client::connect(const std::string& address, int port) {
    int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        return std::unexpected{std::format("Failed to create socket: {}", std::strerror(errno))};
    }

    auto sock_addr = util::make_sockaddr(address, port);
    if (!sock_addr.has_value()) {
        ::close(sock);
        return std::unexpected{
            std::format("Failed to construct address: {}", sock_addr.error().data()),
        };
    }

    if (::connect(sock, (sockaddr*)&sock_addr.value(), sizeof(sock_addr.value())) == -1) {
        ::close(sock);
        return std::unexpected{std::format("Failed to connect: {}", std::strerror(errno))};
    }

    m_sock = sock;

    return {};
}

std::expected<std::string, std::string> Client::command(std::string input) {
    assert(input.size() <= message::constants::MAX_MESSAGE_SIZE);
    Message message_out{input};
    MessageWriter writer{message_out, [&](size_t n, const char* b) {
                             return ::send(m_sock, b, n, 0);
                         }};

    do {
        auto result = writer.write();
        if (!result.has_value()) {
            ::close(m_sock);
            return std::unexpected{result.error()};
        }
    } while (writer.expected_bytes_remaining() > 0);

    // We can move Message into lib, create a MessageWriter and use it above, and use MessageReader
    // here to get the response.

    std::array<char, 1024> buf{};

    MessageReader reader{};

    do {
        ssize_t bytes_recv = ::recv(m_sock, buf.data(), buf.size(), 0);

        if (bytes_recv == 0) {
            ::close(m_sock);
            return {};
        }
        if (bytes_recv == -1) {
            ::close(m_sock);
            err(EXIT_FAILURE, "recv");
        }

        auto ok = reader.process_bytes(bytes_recv, buf.data());
        if (!ok.has_value()) {
            ::close(m_sock);
            return std::unexpected{std::format("Error reading message: {}", ok.error())};
        }
    } while (reader.expected_bytes_remaining() > 0);

    Message message_in{reader.finish()};

    return message_in.message;
}

Client::~Client() {
    if (m_sock > 0)
        ::close(m_sock);
}
