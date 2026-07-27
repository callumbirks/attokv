#ifndef ATTOKV_SERVER_H
#define ATTOKV_SERVER_H

#include "attokv/message.h"
#include "attokv/socket.h"

#include <cstddef>
#include <string>
#include <vector>

namespace attokv {
class Server {
public:
    Server() = default;

    // Start TCP server on given IPv4 address.
    void start(const std::string& address, int port);
    // Accept client connections and process commands until the server is closed.
    void run();
    // Close the listening socket and all client connections.
    void close();

private:
    struct ClientConnection {
        Socket socket;
        MessageReader reader;
        MessageWriter writer;
        bool close_after_write{};
    };

    void accept_clients();
    bool handle_client_events(std::size_t index, short events);

    Socket m_listen_socket;
    std::vector<ClientConnection> m_clients;
};
} // namespace attokv

#endif
