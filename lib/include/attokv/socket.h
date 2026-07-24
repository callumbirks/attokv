#ifndef ATTOKV_SOCKET_H
#define ATTOKV_SOCKET_H

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

    void reset(int fd = -1) noexcept;
    [[nodiscard]]
    int release() noexcept;

private:
    int m_fd{-1};
};

} // namespace attokv

#endif
