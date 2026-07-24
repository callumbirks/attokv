#ifndef ATTOKV_SERVER_H
#define ATTOKV_SERVER_H

#include "attokv/socket.h"
#include "attokv_server/store.h"

namespace attokv {
class Server {
public:
    Server() = default;

    // Start TCP server on given IPv4 address.
    void start(const std::string& address, int port);
    // Accept a client connection on the bound address and process received
    // commands until the connection is closed.
    void handle_client();
    // Close the current client connection.
    void close();

private:
    KVStore m_store;
    Socket m_listen_socket;
};
} // namespace attokv

#endif
