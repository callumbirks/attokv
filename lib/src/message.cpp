#include "attokv/message.h"
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

std::expected<void, std::string> MessageReader::process_bytes(size_t n, const char* bytes) {
    const char* read_bytes = bytes;
    size_t num_bytes = n;
    if (m_expected_size == 0) {
        if (n < 4)
            return std::unexpected{"Invalid message header"};
        uint32_t size{};
        std::memcpy(&size, bytes, 4);

        if (size == 0 || size > message::constants::MAX_MESSAGE_SIZE)
            return std::unexpected{"Invalid message size"};

        m_expected_size = size - 4;
        num_bytes = n - 4;
        m_buffer.reserve(m_expected_size);
        // Skip size header
        read_bytes = bytes + 4;
    }
    if (num_bytes > expected_bytes_remaining())
        return std::unexpected{"Cannot accept more bytes"};
    std::memcpy(m_buffer.data() + m_bytes_read, read_bytes, num_bytes);
    m_bytes_read += num_bytes;
    return {};
}

Message MessageReader::finish() {
    assert(m_expected_size != 0 && m_bytes_read == m_expected_size);

    return {m_buffer.data()};
}

size_t MessageWriter::expected_bytes_remaining() {
    if (m_bytes_written >= m_expected_size)
        return 0;
    else
        return m_expected_size - m_bytes_written;
}

std::expected<void, std::string> MessageWriter::write() {
    if (m_bytes_written == 0) {
        uint32_t size = static_cast<uint32_t>(m_expected_size);
        std::array<char, 4> size_bytes{};
        std::memcpy(size_bytes.data(), &size, 4);
        size_t n_b = m_write_fn(4, size_bytes.data());
        if (n_b != 4) {
            return std::unexpected{"Unable to write header"};
        }
        m_bytes_written = 4;
    }

    size_t bytes_remaining = expected_bytes_remaining();
    if (bytes_remaining == 0)
        return {};

    const char* source{m_message.message.data() + m_bytes_written - 4};

    size_t n_b = m_write_fn(bytes_remaining, source);

    if (n_b <= 0) {
        return std::unexpected{"Unable to write more bytes"};
    }

    m_bytes_written += n_b;

    return {};
}
