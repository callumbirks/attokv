#include "attokv/socket.h"

#include <unistd.h>
#include <utility>

using namespace attokv;

Socket::Socket(Socket&& other) noexcept : m_fd{other.release()} {}

Socket& Socket::operator=(Socket&& other) noexcept {
    if (this != &other)
        reset(other.release());
    return *this;
}

Socket::~Socket() {
    reset();
}

bool Socket::is_valid() const noexcept {
    return m_fd != -1;
}

int Socket::native_handle() const noexcept {
    return m_fd;
}

void Socket::reset(int fd) noexcept {
    if (m_fd == fd)
        return;

    if (m_fd != -1)
        ::close(m_fd);

    m_fd = fd;
}

int Socket::release() noexcept {
    return std::exchange(m_fd, -1);
}
