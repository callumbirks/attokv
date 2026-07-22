#include "attokv/util.h"
#include <expected>

using namespace attokv;

std::expected<sockaddr_in, std::string> util::make_sockaddr(const std::string& address, int port) {
    if (port < 6000 || port > 65535) {
        return std::unexpected{ "Invalid port number" };
    }
    sockaddr_in sock_addr{};
    if (!inet_aton(address.data(), &sock_addr.sin_addr)) {
        return std::unexpected{ "Failed to parse address" };
    }
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(static_cast<uint16_t>(port));
    return sock_addr;
}
