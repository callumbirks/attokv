#include "attokv_server/server.h"
#include "attokv/message.h"
#include "attokv/util.h"
#include "attokv_server/command.h"
#include "attokv_server/executor.h"
#include <arpa/inet.h>
#include <cerrno>
#include <cstdlib>
#include <err.h>
#include <iostream>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <utility>
#include <vector>

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

    auto nonblocking_result = socket.set_nonblocking();
    if (!nonblocking_result) {
        errx(EXIT_FAILURE, "Failed to make listening socket nonblocking: %s",
             nonblocking_result.error().message().c_str());
    }

    m_listen_socket = std::move(socket);
    std::cout << "Listening on " << inet_ntoa(sock_addr.value().sin_addr) << ":" << port << '\n';
}

void Server::run() {
    if (!m_listen_socket.is_valid()) {
        errx(EXIT_FAILURE, "Not bound to socket");
    }

    while (m_listen_socket.is_valid()) {
        std::vector<pollfd> descriptors;
        descriptors.reserve(m_clients.size() + 1);
        descriptors.push_back({
            .fd = m_listen_socket.native_handle(),
            .events = POLLIN,
            .revents = 0,
        });

        for (const auto& client : m_clients) {
            descriptors.push_back({
                .fd = client.socket.native_handle(),
                .events =
                      static_cast<short>(client.writer.has_pending_message() ? POLLOUT : POLLIN),
                .revents = 0,
            });
        }

        int poll_result{};
        do {
            poll_result = ::poll(descriptors.data(), descriptors.size(), -1);
        } while (poll_result == -1 && errno == EINTR);

        if (poll_result == -1)
            err(EXIT_FAILURE, "poll");

        for (std::size_t index = m_clients.size(); index-- > 0;) {
            if (descriptors[index + 1].revents == 0)
                continue;

            if (!handle_client_events(index, descriptors[index + 1].revents)) {
                m_clients.erase(m_clients.begin() + static_cast<std::ptrdiff_t>(index));
            }
        }

        const short listen_events = descriptors.front().revents;
        if ((listen_events & (POLLERR | POLLHUP | POLLNVAL)) != 0)
            errx(EXIT_FAILURE, "Listening socket failed");

        if ((listen_events & POLLIN) != 0)
            accept_clients();
    }
}

void Server::accept_clients() {
    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_addr_size = sizeof(client_addr);
        Socket client_socket{
            ::accept(m_listen_socket.native_handle(), reinterpret_cast<sockaddr*>(&client_addr),
                     &client_addr_size),
        };

        if (!client_socket.is_valid()) {
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return;
            warn("accept");
            return;
        }

        auto nonblocking_result = client_socket.set_nonblocking();
        if (!nonblocking_result) {
            std::cerr << "Failed to make client socket nonblocking: "
                      << nonblocking_result.error().message() << '\n';
            continue;
        }

        auto nodelay_result = client_socket.set_nodelay();
        if (!nodelay_result) {
            std::cerr << "Failed to configure client socket: " << nodelay_result.error().message()
                      << '\n';
            continue;
        }

        m_clients.push_back(ClientConnection{
            .socket = std::move(client_socket),
            .reader = {},
            .writer = {},
            .close_after_write = false,
        });
    }
}

bool Server::handle_client_events(std::size_t index, short events) {
    ClientConnection& client = m_clients[index];

    if ((events & (POLLERR | POLLNVAL)) != 0)
        return false;

    if ((events & POLLIN) != 0) {
        auto read_result = client.reader.read_from(client.socket);
        if (!read_result) {
            log_io_error("Error reading message", read_result.error());
            return false;
        }

        if (read_result->status == MessageReadStatus::closed)
            return false;

        if (read_result->status == MessageReadStatus::complete) {
            CommandResult result = executor::run_command(read_result->message.message);
            if (result.stop && result.output.empty())
                return false;

            auto queue_result = client.writer.queue(Message{std::move(result.output)});
            if (!queue_result) {
                log_io_error("Error queuing message", queue_result.error());
                return false;
            }
            client.close_after_write = result.stop;
        }
    }

    if ((events & POLLOUT) != 0) {
        auto write_result = client.writer.write_to(client.socket);
        if (!write_result) {
            log_io_error("Error writing message", write_result.error());
            return false;
        }

        if (*write_result && client.close_after_write)
            return false;
    }

    if ((events & POLLHUP) != 0)
        return false;

    return true;
}

void Server::close() {
    m_clients.clear();
    m_listen_socket.reset();
}
