#ifndef ATTOKV_SERVER_MESSAGE_H
#define ATTOKV_SERVER_MESSAGE_H

#include <expected>
#include <string>
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
    size_t m_expected_size{ 0 };
    size_t m_bytes_read{ 0 };
    std::vector<char> m_buffer{};
};

} // namespace attokv

#endif
