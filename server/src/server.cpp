#include "attokv_server/server.h"
#include "attokv/message.h"
#include "attokv/util.h"
#include "attokv_server/command.h"
#include "attokv_server/executor.h"
#include <arpa/inet.h>
#include <cstdlib>
#include <err.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>

using namespace attokv;

namespace {

void log_io_error(const char* operation, const IoError& error) {
    std::cerr << operation << ": " << error.context;
    if (error.cause)
        std::cerr << ": " << error.cause.message();
    std::cerr << '\n';
}

} // namespace

void Server::start(const std::string& address, int port) {
    Socket socket{::socket(AF_INET, SOCK_STREAM, 0)};
    if (!socket.is_valid()) {
        err(EXIT_FAILURE, "socket");
    }

    auto sock_addr = util::make_sockaddr(address, port);
    if (!sock_addr.has_value()) {
        err(EXIT_FAILURE, "Failed to construct address: %s", sock_addr.error().data());
    }

    if (::bind(socket.native_handle(), reinterpret_cast<sockaddr*>(&sock_addr.value()),
               sizeof(sock_addr.value())) == -1) {
        err(EXIT_FAILURE, "bind");
    }

    if (::listen(socket.native_handle(), 64) == -1) {
        err(EXIT_FAILURE, "listen");
    }

    m_listen_socket = std::move(socket);
    std::cout << "Listening on " << inet_ntoa(sock_addr.value().sin_addr) << ":" << port << '\n';
}

void Server::handle_client() {
    if (!m_listen_socket.is_valid()) {
        err(EXIT_FAILURE, "Not bound to socket");
    }

    sockaddr_in client_addr{};
    socklen_t client_addr_size = sizeof(client_addr);

    Socket client_socket{::accept(m_listen_socket.native_handle(),
                                  reinterpret_cast<sockaddr*>(&client_addr), &client_addr_size)};
    if (!client_socket.is_valid()) {
        err(EXIT_FAILURE, "accept");
    }

    while (true) {
        auto message = read_message(client_socket);
        if (!message) {
            log_io_error("Error reading message", message.error());
            return;
        }
        if (!*message)
            return;

        CommandResult result = executor::run_command((*message)->message);

        if (!result.output.empty()) {
            auto write_result = write_message(client_socket, Message{result.output});
            if (!write_result) {
                log_io_error("Error writing message", write_result.error());
                return;
            }
        }

        if (result.stop)
            break;
    }
}

void Server::close() {
    m_listen_socket.reset();
}
