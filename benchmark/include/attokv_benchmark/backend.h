#ifndef ATTOKV_BENCHMARK_BACKEND_H
#define ATTOKV_BENCHMARK_BACKEND_H

#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace attokv::benchmark {

enum class BackendKind {
    attokv,
    redis,
};

class Backend {
public:
    virtual ~Backend() = default;

    virtual std::expected<void, std::string> connect(const std::string& host, int port) = 0;
    virtual std::expected<void, std::string> ping() = 0;
    virtual std::expected<void, std::string> flush() = 0;
    virtual std::expected<void, std::string> set(std::string_view key, std::string_view value) = 0;
    virtual std::expected<std::optional<std::string>, std::string> get(std::string_view key) = 0;
    virtual std::expected<bool, std::string> remove(std::string_view key) = 0;
};

std::unique_ptr<Backend> make_backend(BackendKind kind);

} // namespace attokv::benchmark

#endif
