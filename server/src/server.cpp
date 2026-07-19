#include <array>
#include <arpa/inet.h>
#include <cassert>
#include <err.h>
#include <cstdlib>
#include <expected>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "attokv_server/command.h"
#include "attokv_server/executor.h"
#include "attokv_server/message.h"
#include "attokv_server/server.h"

using namespace attokv;

std::expected<sockaddr_in, std::string> make_sockaddr(const std::string& address, int port) {
    if (port < 6000 || port > 65535) {
        return std::unexpected { "Invalid port number" };
    }
    sockaddr_in sock_addr {};
    if (!inet_aton(address.data(), &sock_addr.sin_addr)) {
        return std::unexpected { "Failed to parse address" };
    }
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(static_cast<uint16_t>(port));
    return sock_addr;
}

void Server::start(const std::string& address, int port) {
    int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        err(EXIT_FAILURE, "socket");
    }

    auto sock_addr = make_sockaddr(address, port);
    if (!sock_addr.has_value()) {
        err(EXIT_FAILURE, "Failed to construct address: %s", sock_addr.error().data());
    }

    if(::bind(sock, (sockaddr*) &sock_addr.value(), sizeof(sock_addr.value())) == -1) {
        err(EXIT_FAILURE, "bind");
    }

    if(::listen(sock, 64) == -1) {
        err(EXIT_FAILURE, "listen");
    }

    m_listen_fd = sock;
    std::cout << "Listening on " << inet_ntoa(sock_addr.value().sin_addr) << ":" << port << '\n';
}

void Server::handle_client() {
    if (m_listen_fd <= 0) {
        err(EXIT_FAILURE, "Not bound to socket");
    }

    sockaddr_in client_addr{};
    socklen_t client_addr_size = sizeof(client_addr);

    int client_fd = ::accept(m_listen_fd, (sockaddr*)&client_addr, &client_addr_size);
    if (client_fd == -1) {
        err(EXIT_FAILURE, "accept");
    }

    std::array<char, 1024> buf{};

    while (true) {
        MessageReader reader{};

        do {
            ssize_t bytes_recv = ::recv(client_fd, buf.data(), buf.size(), 0);

            if (bytes_recv == 0) {
                ::close(client_fd);
                return;
            }
            if (bytes_recv == -1) {
                err(EXIT_FAILURE, "recv");
            }

            auto ok = reader.process_bytes(bytes_recv, buf.data());
            if (!ok.has_value()) {
                ::close(client_fd);
                std::cerr << "Error reading message: " << ok.error() << '\n';
                return;
            }

            if (reader.expected_bytes_remaining() > 0 && bytes_recv < 1024) {
                // We are expecting there to be more data on the socket, but there isn't.
                // This avoids the next call to `recv` waiting forever.
                ::close(client_fd);
                std::cerr << "Client did not send enough bytes\n";
                return;
            }
        } while (reader.expected_bytes_remaining() > 0);

        Message message { reader.finish() };

        CommandResult result = executor::run_command(message.message);

        if (!result.output.empty()) {
            // TODO: Write output to socket
            std::cout << result.output << '\n';
        }

        if (result.stop || result.error) break;
    }

    ::close(client_fd);
}

Server::~Server() {
    if (m_listen_fd > 0) ::close(m_listen_fd);
}
