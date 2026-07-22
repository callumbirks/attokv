#ifndef ATTOKV_SERVER_H
#define ATTOKV_SERVER_H

#include "attokv_server/store.h"

namespace attokv {
class Server {
public:
    Server() : m_store{} {}

    // Start TCP server on given IPv4 address.
    void start(const std::string& address, int port);
    // Accept a client connection on the bound address and process received
    // commands until the connection is closed.
    void handle_client();
    // Close the current client connection.
    void close();

    ~Server();

private:
    KVStore m_store;
    int m_listen_fd{0};
};
} // namespace attokv

#endif
