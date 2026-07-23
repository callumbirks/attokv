#ifndef ATTOKV_ERROR_H
#define ATTOKV_ERROR_H

#include <system_error>

enum class IoErrorKind {
    system,
    unexpected_eof,
    invalid_frame,
    message_too_large
};

struct IoError {
    IoErrorKind kind;
    std::error_code cause;
    std::string context;
};

#endif
