#ifndef ATTOKV_CLIENT_H
#define ATTOKV_CLIENT_H

#include <expected>
#include <string>
#include <string_view>

namespace attokv {
class Client {
public:
    Client() {}

    std::expected<void, std::string> connect(const std::string& address, int port);

    std::expected<std::string, std::string> command(std::string_view input);

    ~Client();

private:
    int m_sock{0};
};
} // namespace attokv

#endif
