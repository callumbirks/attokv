#include "attokv_server/executor.h"
#include "attokv_server/command.h"
#include "attokv_server/store.h"
#include <cassert>
#include <format>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

using namespace attokv;

// Should be kept in line with whatever the highest max arg count is out of all commands.
constexpr size_t k_max_command_args = 2;

std::unordered_map<std::string_view, CommandSpec>& commands() {
    static std::unordered_map<std::string_view, CommandSpec> map{};

    return map;
}

KVStore& store() {
    static KVStore store{};
    return store;
}

const CommandSpec* get_command_spec(std::string_view operation) {
    auto& commands = ::commands();
    auto oper = commands.find(operation);
    if (oper == commands.end())
        return nullptr;
    return &oper->second;
}

constexpr bool is_whitespace(char character) {
    switch (character) {
    case ' ':
    case '\t':
    case '\n':
    case '\r':
    case '\f':
    case '\v':
        return true;
    default:
        return false;
    }
}

std::string_view next_token(std::string_view input, std::size_t& position) {
    while (position < input.size() && is_whitespace(input[position])) {
        ++position;
    }

    const size_t begin = position;

    while (position < input.size() && !is_whitespace(input[position])) {
        ++position;
    }

    return input.substr(begin, position - begin);
}

struct ParseResult {
    const CommandSpec* command{};
    std::array<std::string_view, k_max_command_args> args{};
    size_t arg_count{};
    std::string error{};
};

ParseResult parseCommand(std::string_view input) {

    size_t position{};

    const std::string_view operation = next_token(input, position);

    if (operation.empty())
        return {.error = "Empty input"};

    const CommandSpec* command = get_command_spec(operation);

    if (!command) {
        return {.error = std::format("No such command '{}'", operation)};
    }

    assert(command->required_args >= 0);
    assert(static_cast<size_t>(command->required_args) <= k_max_command_args);

    ParseResult result{.command = command};

    while (true) {
        const std::string_view argument = next_token(input, position);

        if (argument.empty())
            break;

        if (result.arg_count == result.args.size()) {
            return {.error = std::format("Invalid number of args for command '{}'", operation)};
        }

        result.args[result.arg_count++] = argument;
    }

    if (result.arg_count != static_cast<size_t>(command->required_args)) {
        return {.error = std::format("Invalid number of args for command '{}'", operation)};
    }

    return result;
}

CommandResult executor::run_command(std::string_view input) {
    ParseResult parsed = parseCommand(input);

    if (!parsed.command) {
        return {.error = true, .output = std::move(parsed.error)};
    }

    CommandContext context{.store = &store(), .args = {parsed.args.data(), parsed.arg_count}};

    return parsed.command->run(context);
}

void executor::register_builtins() {
    auto& commands = ::commands();

    commands.reserve(command::builtin().size());

    for (const auto& spec : command::builtin()) {
        commands.emplace(spec.name, spec);
    }
}
