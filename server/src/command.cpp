#include <cassert>
#include <string_view>
#include "attokv_server/command.h"

using namespace attokv;

CommandResult command::_builtin_exit(CommandContext _) {
    return { .stop = true };
}

CommandResult command::_builtin_ping(CommandContext _) {
    return { .output = "pong" };
}

CommandResult command::_builtin_get(CommandContext context) {
    std::string_view val { context.store->get(context.args[0]) };
    if (val.empty()) return { .output = "NULL" };
    else return { .output = std::string{val} };
}

CommandResult command::_builtin_set(CommandContext context) {
    context.store->set(context.args[0], context.args[1]);
    return { .output = "OK" };
}

CommandResult command::_builtin_flush(CommandContext context) {
    context.store->flush();
    return { .output = "OK" };
}

const std::array<CommandSpec, 5>& command::builtin() {
    static const std::array<CommandSpec, 5> array {{
        { "exit", 0, _builtin_exit },
        { "ping", 0, _builtin_ping },
        { "get", 1, _builtin_get },
        { "set", 2, _builtin_set },
        { "flush", 0, _builtin_flush }
    }};
    return array;
}
