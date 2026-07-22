#ifndef ATTOKV_MESSAGE_H
#define ATTOKV_MESSAGE_H

#include <expected>
#include <functional>
#include <string>
#include <sys/types.h>
#include <vector>

namespace attokv {

namespace message::constants {
static constexpr size_t MAX_MESSAGE_SIZE = 16'777'215;
};

struct Message {
    std::string message;
};

class MessageReader {
public:
    MessageReader() {}

    size_t expected_bytes_remaining();

    std::expected<void, std::string> process_bytes(size_t n, const char* bytes);

    Message finish();

private:
    size_t m_expected_size{0};
    size_t m_bytes_read{0};
    std::vector<char> m_buffer{};
};

class MessageWriter {
public:
    MessageWriter(Message message, std::function<ssize_t(size_t, char*)> write_fn)
        : m_message{message}, m_write_fn{write_fn} {}

    size_t expected_bytes_remaining();

    void write();

private:
    Message m_message;
    std::function<ssize_t(size_t, char*)> m_write_fn;
    size_t m_expected_size{0};
    size_t m_bytes_written{0};
};

} // namespace attokv

#endif
