#ifndef ATTOKV_SOCKET_H
#define ATTOKV_SOCKET_H

#include <expected>
#include <system_error>

namespace attokv {

class Socket {
public:
    Socket() noexcept = default;
    explicit Socket(int fd) noexcept : m_fd{fd} {}

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;

    ~Socket();

    [[nodiscard]]
    bool is_valid() const noexcept;
    [[nodiscard]]
    int native_handle() const noexcept;

    std::expected<void, std::error_code> set_nonblocking(bool enabled = true) const noexcept;
    std::expected<void, std::error_code> set_nodelay(bool enabled = true) const noexcept;

    void reset(int fd = -1) noexcept;
    [[nodiscard]]
    int release() noexcept;

private:
    int m_fd{-1};
};

} // namespace attokv

#endif
