#ifndef ATTOKV_CLIENT_H
#define ATTOKV_CLIENT_H

#include "attokv/error.h"
#include "attokv/socket.h"

#include <expected>
#include <optional>
#include <string>

namespace attokv {
class Client {
public:
    Client() = default;

    std::expected<void, std::string> connect(const std::string& address, int port);

    std::expected<std::optional<std::string>, IoError> command(std::string input);

private:
    Socket m_socket;
};
} // namespace attokv

#endif
