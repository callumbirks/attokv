#ifndef ATTOKV_SERVER_EXECUTOR_H
#define ATTOKV_SERVER_EXECUTOR_H

#include "attokv_server/command.h"

namespace attokv::executor {
    void register_builtins();
    CommandResult run_command(const std::string& input);
}

#endif
