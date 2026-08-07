#include "attokv_server/command.h"
#include "attokv_server/executor.h"
#include "attokv_server/server.h"
#include "attokv_server/store.h"
#include <cassert>
#include <charconv>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>

using namespace attokv;

void run_repl() {
    std::string line{};
    KVStore store{};

    while (true) {
        std::cout << "attokv> ";

        if (!std::getline(std::cin, line)) {
            break;
        }

        CommandResult result = executor::run_command(line);

        if (!result.output.empty()) {
            std::cout << result.output << '\n';
        }

        if (result.error)
            continue;

        if (result.stop)
            break;
    }
}

void run_server(const std::string& host, int port) {
    Server server{};
    server.start(host, port);
    server.run();
}

struct Options {
    std::string host{"127.0.0.1"};
    int port{6337};
    bool repl{};
};

void print_usage() {
    std::cout << "Usage: attokv_server [--host <IPv4 address>] [--port <port>] [--repl]\n";
}

bool parse_port(std::string_view input, int& port) {
    const char* end = input.data() + input.size();
    auto result = std::from_chars(input.data(), end, port);
    return result.ec == std::errc{} && result.ptr == end && port >= 1 && port <= 65535;
}

bool parse_options(int argc, const char** argv, Options& options, bool& help) {
    for (int i = 1; i < argc; i++) {
        std::string_view argument{argv[i]};
        if (argument == "--help") {
            help = true;
            return true;
        }
        if (argument == "--repl") {
            options.repl = true;
            continue;
        }
        if (argument == "--host" && i + 1 < argc) {
            options.host = argv[++i];
            continue;
        }
        if (argument == "--port" && i + 1 < argc && parse_port(argv[i + 1], options.port)) {
            i++;
            continue;
        }
        std::cerr << "Invalid argument: " << argument << '\n';
        return false;
    }
    return true;
}

int main(int argc, const char** argv) {
    Options options{};
    bool help{};
    if (!parse_options(argc, argv, options, help)) {
        print_usage();
        return 1;
    }
    if (help) {
        print_usage();
        return 0;
    }

    executor::register_builtins();

    if (options.repl) {
        std::cout << "Repl mode enabled\n";
        run_repl();
    } else {
        run_server(options.host, options.port);
    }

    return 0;
}
