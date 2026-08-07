#include "attokv/socket.h"

#include <cerrno>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <system_error>
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

std::expected<void, std::error_code> Socket::set_nonblocking(bool enabled) const noexcept {
    const int flags = ::fcntl(m_fd, F_GETFL);
    if (flags == -1)
        return std::unexpected{std::error_code{errno, std::generic_category()}};

    const int updated_flags = enabled ? flags | O_NONBLOCK : flags & ~O_NONBLOCK;
    if (::fcntl(m_fd, F_SETFL, updated_flags) == -1)
        return std::unexpected{std::error_code{errno, std::generic_category()}};

    return {};
}

std::expected<void, std::error_code> Socket::set_nodelay(bool enabled) const noexcept {
    int value = enabled ? 1 : 0;
    if (::setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value)) == -1)
        return std::unexpected{std::error_code{errno, std::generic_category()}};
    return {};
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
