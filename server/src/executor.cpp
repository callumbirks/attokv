#include "attokv_server/executor.h"
#include "attokv_server/command.h"
#include "attokv_server/store.h"
#include <format>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

using namespace attokv;

std::unordered_map<std::string_view, CommandSpec>& commands() {
    static std::unordered_map<std::string_view, CommandSpec> map{};

    return map;
}

KVStore& store() {
    static KVStore store{};
    return store;
}

std::optional<CommandSpec> getCommandSpec(const std::string& oper_str) {
    auto& commands = ::commands();
    auto oper = commands.find(oper_str);
    if (oper == commands.end())
        return std::nullopt;
    return oper->second;
}

struct ParseResult {
    std::optional<CommandSpec> command{};
    std::vector<std::string> args{};
    std::string error{};
};

ParseResult parseCommand(const std::string& line) {
    std::istringstream input{line};

    std::string oper_str{};
    if (!(input >> oper_str)) {
        return {.error = "Empty input"};
    }

    std::optional<CommandSpec> command{getCommandSpec(oper_str)};
    if (!command)
        return {.error = std::format("No such command '{}'", oper_str)};

    std::vector<std::string> args;
    args.reserve(command->required_args);
    std::string arg{};

    for (int i = 0; input >> arg; i++) {
        if (i == command->required_args) {
            return {.error = std::format("Invalid number of args for command '{}'", oper_str)};
        }
        args.push_back(arg);
    }

    if (command->required_args > -1 && static_cast<int>(args.size()) != command->required_args) {
        return {.error = std::format("Invalid number of args for command '{}'", oper_str)};
    }

    return {.command = command, .args = args};
}

CommandResult executor::run_command(const std::string& input) {
    ParseResult command{parseCommand(input)};

    if (!command.command) {
        return {.error = true, .output = command.error};
    }

    CommandContext context{&store(), static_cast<int>(command.args.size()), command.args.data()};

    return command.command->run(context);
}

void executor::register_builtins() {
    auto& commands = ::commands();
    for (const auto& spec : command::builtin()) {
        commands.emplace(spec.name, spec);
    }
}
