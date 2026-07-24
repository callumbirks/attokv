#include "attokv_client/client.h"
#include <iostream>
#include <string>

using namespace attokv;

int main() {
    std::string line{};
    Client client{};

    auto conn_result = client.connect("127.0.0.1", 6338);
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
