#include "attokv/error.h"
#include "attokv_benchmark/backend.h"
#include "attokv_client/client.h"
#include <expected>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace attokv::benchmark {
namespace {

std::string describe_error(const IoError& error) {
    if (error.cause)
        return std::format("{}: {}", error.context, error.cause.message());
    return error.context;
}

class AttoBackend final : public Backend {
public:
    std::expected<void, std::string> connect(const std::string& host, int port) override {
        return m_client.connect(host, port);
    }

    std::expected<void, std::string> ping() override {
        auto result = call("ping");
        if (!result)
            return std::unexpected{std::move(result.error())};
        if (*result != "pong")
            return std::unexpected{std::format("Unexpected PING response: {}", *result)};
        return {};
    }

    std::expected<void, std::string> flush() override {
        return expect_ok(call("flush"));
    }

    std::expected<void, std::string> set(std::string_view key, std::string_view value) override {
        return expect_ok(call(std::format("set {} {}", key, value)));
    }

    std::expected<std::optional<std::string>, std::string> get(std::string_view key) override {
        auto result = call(std::format("get {}", key));
        if (!result)
            return std::unexpected{std::move(result.error())};
        if (*result == "NULL")
            return std::optional<std::string>{};
        return std::optional<std::string>{std::move(*result)};
    }

    std::expected<bool, std::string> remove(std::string_view key) override {
        auto result = call(std::format("del {}", key));
        if (!result)
            return std::unexpected{std::move(result.error())};
        if (*result == "OK")
            return true;
        if (*result == "NULL")
            return false;
        return std::unexpected{std::format("Unexpected DEL response: {}", *result)};
    }

private:
    std::expected<std::string, std::string> call(std::string command) {
        auto result = m_client.command(std::move(command));
        if (!result)
            return std::unexpected{describe_error(result.error())};
        if (!*result)
            return std::unexpected{"Server closed the connection"};
        return std::move(**result);
    }

    std::expected<void, std::string> expect_ok(std::expected<std::string, std::string> result) {
        if (!result)
            return std::unexpected{std::move(result.error())};
        if (*result != "OK")
            return std::unexpected{std::format("Unexpected response: {}", *result)};
        return {};
    }

    Client m_client{};
};

} // namespace

std::unique_ptr<Backend> make_attokv_backend() {
    return std::make_unique<AttoBackend>();
}

} // namespace attokv::benchmark
