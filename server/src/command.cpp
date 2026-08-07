#include "attokv_server/command.h"

using namespace attokv;

CommandResult command::_builtin_exit(CommandContext) {
    return {.stop = true};
}

CommandResult command::_builtin_ping(CommandContext) {
    return {.output = "pong"};
}

CommandResult command::_builtin_get(CommandContext context) {
    auto val = context.store->get(context.args[0]);
    if (!val)
        return {.output = "NULL"};
    else
        return {.output = std::string{*val}};
}

CommandResult command::_builtin_set(CommandContext context) {
    context.store->set(context.args[0], context.args[1]);
    return {.output = "OK"};
}

CommandResult command::_builtin_del(CommandContext context) {
    bool removed = context.store->remove(context.args[0]);
    if (removed)
        return {.output = "OK"};
    else
        return {.output = "NULL"};
}

CommandResult command::_builtin_flush(CommandContext context) {
    context.store->flush();
    return {.output = "OK"};
}

const std::array<CommandSpec, 6>& command::builtin() {
    static const std::array<CommandSpec, 6> array{
        {{"exit", 0, _builtin_exit},
         {"ping", 0, _builtin_ping},
         {"get", 1, _builtin_get},
         {"set", 2, _builtin_set},
         {"del", 1, _builtin_del},
         {"flush", 0, _builtin_flush}}
    };
    return array;
}
