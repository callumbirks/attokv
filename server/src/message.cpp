#include "attokv_server/message.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <expected>

using namespace attokv;

size_t MessageReader::expected_bytes_remaining() {
    if (m_bytes_read >= m_expected_size)
        return 0;
    else
        return m_expected_size - m_bytes_read;
}

std::expected<void, std::string>
MessageReader::process_bytes(size_t n, const char* bytes) {
    const char* read_bytes = bytes;
    size_t num_bytes = n;
    if (m_expected_size == 0) {
        if (n < 4)
            return std::unexpected{ "Invalid message header" };
        uint32_t size{};
        std::memcpy(&size, bytes, 4);

        if (size == 0 || size > message::constants::MAX_MESSAGE_SIZE)
            return std::unexpected{ "Invalid message size" };

        m_expected_size = size - 4;
        num_bytes = n - 4;
        m_buffer.reserve(m_expected_size);
        // Skip size header
        read_bytes = bytes + 4;
    }
    if (num_bytes > expected_bytes_remaining())
        return std::unexpected{ "Cannot accept more bytes" };
    std::memcpy(m_buffer.data() + m_bytes_read, read_bytes, num_bytes);
    m_bytes_read += num_bytes;
    return {};
}

Message MessageReader::finish() {
    assert(m_expected_size != 0 && m_bytes_read == m_expected_size);

    return { m_buffer.data() };
}
