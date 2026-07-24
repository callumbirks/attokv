#include "attokv_client/client.h"
#include "attokv/message.h"
#include "attokv/util.h"
#include <cerrno>
#include <cstring>
#include <expected>
#include <format>
#include <netinet/in.h>
#include <sys/socket.h>
#include <utility>

using namespace attokv;

std::expected<void, std::string> Client::connect(const std::string& address, int port) {
    Socket socket{::socket(AF_INET, SOCK_STREAM, 0)};
    if (!socket.is_valid()) {
        return std::unexpected{std::format("Failed to create socket: {}", std::strerror(errno))};
    }

    auto sock_addr = util::make_sockaddr(address, port);
    if (!sock_addr.has_value()) {
        return std::unexpected{
            std::format("Failed to construct address: {}", sock_addr.error().data()),
        };
    }

    if (::connect(socket.native_handle(), reinterpret_cast<sockaddr*>(&sock_addr.value()),
                  sizeof(sock_addr.value())) == -1) {
        return std::unexpected{std::format("Failed to connect: {}", std::strerror(errno))};
    }

    m_socket = std::move(socket);
    return {};
}

std::expected<std::optional<std::string>, IoError> Client::command(std::string input) {
    auto write_result = write_message(m_socket, Message{std::move(input)});
    if (!write_result) {
        m_socket.reset();
        return std::unexpected{std::move(write_result.error())};
    }

    auto read_result = read_message(m_socket);
    if (!read_result) {
        m_socket.reset();
        return std::unexpected{std::move(read_result.error())};
    }
    if (!*read_result) {
        m_socket.reset();
        return std::optional<std::string>{};
    }

    return std::optional<std::string>{std::move((*read_result)->message)};
}
