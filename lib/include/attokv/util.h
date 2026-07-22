#ifndef ATTOKV_UTIL_H
#define ATTOKV_UTIL_H

#include <arpa/inet.h>
#include <expected>
#include <string>
#include <sys/socket.h>

namespace attokv::util {
std::expected<sockaddr_in, std::string> make_sockaddr(const std::string& address, int port);
}

#endif
