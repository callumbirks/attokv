#ifndef ATTOKV_SERVER_COMMAND_H
#define ATTOKV_SERVER_COMMAND_H

#include <array>
#include <functional>
#include <string>

#include "attokv_server/store.h"

namespace attokv {

struct CommandResult {
    bool stop { false };
    bool error { false };
    std::string output {};
};

struct CommandContext {
    KVStore* store;
    int n_args;
    const std::string* args;
};

struct CommandSpec {
    std::string name;
    /* If a command has an exact number of required args, this can be specified to pre-validate before calling `run`.
     * If `required_args` is `-1`, no pre-validation of arg count will occur.
     */
    int required_args;
    std::function<CommandResult(CommandContext)> run;
};

namespace command {
CommandResult _builtin_exit(CommandContext context);
CommandResult _builtin_ping(CommandContext context);
CommandResult _builtin_get(CommandContext context);
CommandResult _builtin_set(CommandContext context);
CommandResult _builtin_flush(CommandContext context);

const std::array<CommandSpec, 5>& builtin();
}
}

#endif
