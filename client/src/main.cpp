#include "attokv_client/client.h"
#include <charconv>
#include <iostream>
#include <string>
#include <string_view>

using namespace attokv;

namespace {

struct Options {
    std::string host{"127.0.0.1"};
    int port{6337};
};

void print_usage() {
    std::cout << "Usage: attokv_client [--host <IPv4 address>] [--port <port>]\n";
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
        if (argument == "--host" && i + 1 < argc) {
            options.host = argv[++i];
            continue;
        }
        if (argument == "--port" && i + 1 < argc && parse_port(argv[i + 1], options.port)) {
            i++;
            continue;
        }
        std::cerr << "Invalid argument: " << argument << '\n';
        print_usage();
        return false;
    }
    return true;
}

} // namespace

int main(int argc, const char** argv) {
    Options options{};
    bool help{};
    if (!parse_options(argc, argv, options, help))
        return 1;
    if (help) {
        print_usage();
        return 0;
    }

    std::string line{};
    Client client{};

    auto conn_result = client.connect(options.host, options.port);
    if (!conn_result.has_value()) {
        std::cerr << "Failed to connect: " << conn_result.error() << '\n';
        return 1;
    }

    while (true) {
        std::cout << "attokv> ";

        if (!std::getline(std::cin, line)) {
            break;
        }

        auto result = client.command(line);
        if (!result.has_value()) {
            const auto& error = result.error();
            std::cerr << "Failed to call command: " << error.context;
            if (error.cause)
                std::cerr << ": " << error.cause.message();
            std::cerr << '\n';
            return 1;
        }

        if (!result.value()) {
            break;
        }

        std::cout << *result.value() << '\n';
    }

    return 0;
}
