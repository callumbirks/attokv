#include "attokv_client/client.h"
#include "attokv/util.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <err.h>
#include <expected>
#include <format>
#include <netinet/in.h>
#include <string_view>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

using namespace attokv;

std::expected<void, std::string> Client::connect(const std::string& address, int port) {
    int sock = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        return std::unexpected{std::format("Failed to create socket: {}", std::strerror(errno))};
    }

    auto sock_addr = util::make_sockaddr(address, port);
    if (!sock_addr.has_value()) {
        return std::unexpected{
            std::format("Failed to construct address: {}", sock_addr.error().data()),
        };
    }

    if (::connect(sock, (sockaddr*)&sock_addr.value(), htons(port)) == -1) {
        return std::unexpected{std::format("Failed to connect: {}", std::strerror(errno))};
    }

    m_sock = sock;

    return {};
}

std::expected<std::string, std::string> Client::command(std::string_view input) {
    // assert(input.size() <= message::constants::MAX_MESSAGE_SIZE)
    uint32_t size = 4 + input.size();
    std::vector<char> bytes{};
    bytes.resize(size);
    std::memcpy(bytes.data(), &size, 4);
    std::memcpy(bytes.data() + 4, input.data(), input.size());

    ssize_t bytes_sent = ::send(m_sock, bytes.data(), size, 0);
    if (bytes_sent == -1) {
        err(EXIT_FAILURE, "send");
    }

    // We can move Message into lib, create a MessageWriter and use it above, and use MessageReader
    // here to get the response.
}

Client::~Client() {
    if (m_sock > 0)
        ::close(m_sock);
}
