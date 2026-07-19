#include <cassert>
#include <iostream>
#include <sstream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <format>
#include "kvcli/store.h"
#include "kvcli/command.h"

using namespace kvcli;

std::unordered_map<std::string_view, CommandSpec>& commands() {
    static std::unordered_map<std::string_view, CommandSpec> map {};

    return map;
}

std::optional<CommandSpec> getCommandSpec(const std::string& oper_str) {
    auto& commands = ::commands();
    auto oper = commands.find(oper_str);
    if (oper == commands.end()) return std::nullopt;
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
        return { .error = "Empty input" };
    }

    std::optional<CommandSpec> command { getCommandSpec(oper_str) };
    if (!command) return { .error = std::format("No such command '{}'", oper_str) };

    std::vector<std::string> args;
    args.reserve(command->required_args);
    std::string arg{};

    for (int i = 0; input >> arg; i++) {
        // Early catch to avoid processing loads of args
        if (i == command->required_args) {
            return { .error = std::format("Invalid number of args for command '{}'", oper_str) };
        }
        args.push_back(arg);
    }

    return { .command = command, .args = args };
}

void register_builtins() {
    auto& commands = ::commands();
    for (const auto& spec : command::builtin()) {
        commands.emplace(spec.name, spec);
    }
}

int main() {
    std::string line{};
    KVStore store{};

    register_builtins();

    while (true) {
        std::cout << "kvcli> ";

        if (!std::getline(std::cin, line)) {
            break;
        }

        ParseResult command = parseCommand(line);

        if (!command.command) {
            std::cout << command.error << '\n';
            continue;
        }

        CommandContext context { &store, static_cast<int>(command.args.size()), command.args.data() };
        CommandResult result = command.command->run(context);

        if (!result.output.empty()) {
            std::cout << result.output << '\n';
        }

        if (result.stop) break;
    }

    return 0;
}
