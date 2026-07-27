#ifndef ATTOKV_MESSAGE_H
#define ATTOKV_MESSAGE_H

#include "attokv/error.h"
#include "attokv/socket.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>

namespace attokv {

namespace message::constants {
inline constexpr std::size_t MAX_MESSAGE_SIZE = 16'777'215;
}

struct Message {
    std::string message;
};

enum class MessageReadStatus {
    incomplete,
    closed,
    complete
};

struct MessageReadResult {
    MessageReadStatus status;
    Message message;
};

class MessageReader {
public:
    std::expected<MessageReadResult, IoError> read_from(const Socket& socket);

private:
    void reset();

    std::array<char, sizeof(std::uint32_t)> m_header{};
    std::size_t m_header_bytes{};
    std::string m_payload{};
    std::size_t m_payload_bytes{};
};

class MessageWriter {
public:
    [[nodiscard]]
    bool has_pending_message() const noexcept;

    std::expected<void, IoError> queue(Message message);
    std::expected<bool, IoError> write_to(const Socket& socket);

private:
    void reset();

    std::array<char, sizeof(std::uint32_t)> m_header{};
    std::size_t m_header_bytes{};
    std::string m_payload{};
    std::size_t m_payload_bytes{};
    bool m_pending{};
};

std::expected<std::optional<Message>, IoError> read_message(const Socket& socket);
std::expected<void, IoError> write_message(const Socket& socket, const Message& message);

} // namespace attokv

#endif
