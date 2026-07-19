#include <cassert>
#include <cstring>
#include <iostream>
#include <string>

#include "attokv_server/executor.h"
#include "attokv_server/server.h"
#include "attokv_server/store.h"
#include "attokv_server/command.h"

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

        if (result.error) continue;

        if (result.stop) break;
    }
}

void run_server() {
    Server server{};
    server.start("127.0.0.1", 6337);
    while (true) {
        server.handle_client();
    }
}

int main(int argc, const char** argv) {
    executor::register_builtins();

    bool repl { false };
    if (argc > 1 && strcmp(argv[1], "--repl") == 0) {
        repl = true;
        std::cout << "Repl mode enabled\n";
    }

    if (repl) run_repl();
    else run_server();

    return 0;
}
