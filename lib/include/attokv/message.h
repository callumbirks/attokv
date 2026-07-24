#ifndef ATTOKV_MESSAGE_H
#define ATTOKV_MESSAGE_H

#include "attokv/error.h"
#include "attokv/socket.h"

#include <cstddef>
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

std::expected<std::optional<Message>, IoError> read_message(const Socket& socket);
std::expected<void, IoError> write_message(const Socket& socket, const Message& message);

} // namespace attokv

#endif
