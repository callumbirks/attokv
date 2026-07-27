#include "attokv/message.h"

#include <array>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <expected>
#include <netinet/in.h>
#include <span>
#include <sys/socket.h>
#include <system_error>
#include <utility>

using namespace attokv;

namespace {

IoError system_error(std::string context) {
    return {
        .kind = IoErrorKind::system,
        .cause = {errno, std::generic_category()},
        .context = std::move(context),
    };
}

IoError unexpected_eof(std::string context) {
    return {
        .kind = IoErrorKind::unexpected_eof,
        .cause = {},
        .context = std::move(context),
    };
}

bool would_block() {
    return errno == EAGAIN || errno == EWOULDBLOCK;
}

std::expected<bool, IoError> read_exact(const Socket& socket, std::span<char> destination,
                                        const char* context) {
    std::size_t bytes_read = 0;

    while (bytes_read < destination.size()) {
        const ssize_t result = ::recv(socket.native_handle(), destination.data() + bytes_read,
                                      destination.size() - bytes_read, 0);

        if (result > 0) {
            bytes_read += static_cast<std::size_t>(result);
            continue;
        }

        if (result == 0) {
            if (bytes_read == 0)
                return false;
            return std::unexpected{unexpected_eof(context)};
        }

        if (errno == EINTR)
            continue;

        return std::unexpected{system_error(context)};
    }

    return true;
}

std::expected<void, IoError> write_all(const Socket& socket, std::span<const char> source,
                                       const char* context) {
    std::size_t bytes_written = 0;

    while (bytes_written < source.size()) {
        const ssize_t result = ::send(socket.native_handle(), source.data() + bytes_written,
                                      source.size() - bytes_written, MSG_NOSIGNAL);

        if (result > 0) {
            bytes_written += static_cast<std::size_t>(result);
            continue;
        }

        if (result == 0) {
            return std::unexpected{IoError{
                .kind = IoErrorKind::system,
                .cause = std::make_error_code(std::errc::broken_pipe),
                .context = context,
            }};
        }

        if (errno == EINTR)
            continue;

        return std::unexpected{system_error(context)};
    }

    return {};
}

} // namespace

std::expected<MessageReadResult, IoError> MessageReader::read_from(const Socket& socket) {
    while (m_header_bytes < m_header.size()) {
        const ssize_t result =
            ::recv(socket.native_handle(), m_header.data() + m_header_bytes,
                   m_header.size() - m_header_bytes, 0);

        if (result > 0) {
            m_header_bytes += static_cast<std::size_t>(result);
            continue;
        }

        if (result == 0) {
            if (m_header_bytes == 0)
                return MessageReadResult{
                    .status = MessageReadStatus::closed,
                    .message = {},
                };
            return std::unexpected{unexpected_eof("Failed to read message header")};
        }

        if (errno == EINTR)
            continue;
        if (would_block())
            return MessageReadResult{
                .status = MessageReadStatus::incomplete,
                .message = {},
            };
        return std::unexpected{system_error("Failed to read message header")};
    }

    if (m_payload.empty() && m_payload_bytes == 0) {
        std::uint32_t network_size{};
        std::memcpy(&network_size, m_header.data(), m_header.size());
        const std::uint32_t payload_size = ntohl(network_size);

        if (payload_size > message::constants::MAX_MESSAGE_SIZE) {
            return std::unexpected{IoError{
                .kind = IoErrorKind::message_too_large,
                .cause = {},
                .context = "Incoming message exceeds maximum size",
            }};
        }

        m_payload.resize(payload_size);
        if (payload_size == 0) {
            reset();
            return MessageReadResult{.status = MessageReadStatus::complete, .message = Message{}};
        }
    }

    while (m_payload_bytes < m_payload.size()) {
        const ssize_t result =
            ::recv(socket.native_handle(), m_payload.data() + m_payload_bytes,
                   m_payload.size() - m_payload_bytes, 0);

        if (result > 0) {
            m_payload_bytes += static_cast<std::size_t>(result);
            continue;
        }

        if (result == 0) {
            return std::unexpected{
                unexpected_eof("Peer closed before sending the message payload")};
        }

        if (errno == EINTR)
            continue;
        if (would_block())
            return MessageReadResult{
                .status = MessageReadStatus::incomplete,
                .message = {},
            };
        return std::unexpected{system_error("Failed to read message payload")};
    }

    Message message{std::move(m_payload)};
    reset();
    return MessageReadResult{
        .status = MessageReadStatus::complete,
        .message = std::move(message),
    };
}

void MessageReader::reset() {
    m_header = {};
    m_header_bytes = 0;
    m_payload.clear();
    m_payload_bytes = 0;
}

bool MessageWriter::has_pending_message() const noexcept {
    return m_pending;
}

std::expected<void, IoError> MessageWriter::queue(Message message) {
    if (m_pending) {
        return std::unexpected{IoError{
            .kind = IoErrorKind::invalid_frame,
            .cause = {},
            .context = "Cannot queue a message while another message is pending",
        }};
    }

    if (message.message.size() > message::constants::MAX_MESSAGE_SIZE) {
        return std::unexpected{IoError{
            .kind = IoErrorKind::message_too_large,
            .cause = {},
            .context = "Outgoing message exceeds maximum size",
        }};
    }

    const std::uint32_t network_size =
        htonl(static_cast<std::uint32_t>(message.message.size()));
    std::memcpy(m_header.data(), &network_size, m_header.size());
    m_payload = std::move(message.message);
    m_pending = true;
    return {};
}

std::expected<bool, IoError> MessageWriter::write_to(const Socket& socket) {
    if (!m_pending)
        return true;

    while (m_header_bytes < m_header.size()) {
        const ssize_t result =
            ::send(socket.native_handle(), m_header.data() + m_header_bytes,
                   m_header.size() - m_header_bytes, MSG_NOSIGNAL);

        if (result > 0) {
            m_header_bytes += static_cast<std::size_t>(result);
            continue;
        }

        if (result == 0) {
            return std::unexpected{IoError{
                .kind = IoErrorKind::system,
                .cause = std::make_error_code(std::errc::broken_pipe),
                .context = "Failed to write message header",
            }};
        }

        if (errno == EINTR)
            continue;
        if (would_block())
            return false;
        return std::unexpected{system_error("Failed to write message header")};
    }

    while (m_payload_bytes < m_payload.size()) {
        const ssize_t result =
            ::send(socket.native_handle(), m_payload.data() + m_payload_bytes,
                   m_payload.size() - m_payload_bytes, MSG_NOSIGNAL);

        if (result > 0) {
            m_payload_bytes += static_cast<std::size_t>(result);
            continue;
        }

        if (result == 0) {
            return std::unexpected{IoError{
                .kind = IoErrorKind::system,
                .cause = std::make_error_code(std::errc::broken_pipe),
                .context = "Failed to write message payload",
            }};
        }

        if (errno == EINTR)
            continue;
        if (would_block())
            return false;
        return std::unexpected{system_error("Failed to write message payload")};
    }

    reset();
    return true;
}

void MessageWriter::reset() {
    m_header = {};
    m_header_bytes = 0;
    m_payload.clear();
    m_payload_bytes = 0;
    m_pending = false;
}

std::expected<std::optional<Message>, IoError> attokv::read_message(const Socket& socket) {
    std::array<char, sizeof(std::uint32_t)> header{};
    auto header_result = read_exact(socket, header, "Failed to read message header");
    if (!header_result)
        return std::unexpected{std::move(header_result.error())};
    if (!*header_result)
        return std::optional<Message>{};

    std::uint32_t network_size{};
    std::memcpy(&network_size, header.data(), header.size());
    const std::uint32_t payload_size = ntohl(network_size);

    if (payload_size > message::constants::MAX_MESSAGE_SIZE) {
        return std::unexpected{IoError{
            .kind = IoErrorKind::message_too_large,
            .cause = {},
            .context = "Incoming message exceeds maximum size",
        }};
    }

    Message message{std::string(payload_size, '\0')};
    if (payload_size != 0) {
        auto payload_result = read_exact(socket, message.message, "Failed to read message payload");
        if (!payload_result)
            return std::unexpected{std::move(payload_result.error())};
        if (!*payload_result)
            return std::unexpected{unexpected_eof(
                  "Peer closed before sending the message payload")};
    }

    return std::optional<Message>{std::move(message)};
}

std::expected<void, IoError> attokv::write_message(const Socket& socket, const Message& message) {
    if (message.message.size() > message::constants::MAX_MESSAGE_SIZE) {
        return std::unexpected{IoError{
            .kind = IoErrorKind::message_too_large,
            .cause = {},
            .context = "Outgoing message exceeds maximum size",
        }};
    }

    const std::uint32_t network_size = htonl(static_cast<std::uint32_t>(message.message.size()));
    std::array<char, sizeof(network_size)> header{};
    std::memcpy(header.data(), &network_size, header.size());

    auto header_result = write_all(socket, header, "Failed to write message header");
    if (!header_result)
        return header_result;

    return write_all(socket, message.message, "Failed to write message payload");
}
