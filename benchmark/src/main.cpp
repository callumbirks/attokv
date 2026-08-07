#include "attokv_benchmark/backend.h"
#include <algorithm>
#include <barrier>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <expected>
#include <format>
#include <iostream>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

using namespace attokv::benchmark;

namespace {

enum class Workload {
    ping,
    set,
    get_hit,
    get_miss,
    del_hit,
    mixed,
};

enum class OperationKind {
    ping,
    set,
    get_hit,
    get_miss,
    remove,
};

struct Operation {
    OperationKind kind;
    std::string key{};
};

struct Options {
    std::optional<BackendKind> backend{};
    std::optional<Workload> workload{};
    std::string host{"127.0.0.1"};
    int port{};
    bool port_set{};
    std::size_t clients{1};
    std::size_t requests{100'000};
    std::size_t warmup{10'000};
    std::size_t keyspace{10'000};
    std::size_t value_size{256};
    unsigned get_ratio{80};
    std::uint64_t seed{1};
};

struct PhaseResult {
    std::chrono::nanoseconds elapsed{};
    std::vector<std::uint64_t> latencies{};
    std::string error{};
};

void print_usage() {
    std::cout << "Usage: attokv_benchmark --backend <attokv|redis> --workload <name> [options]\n"
              << "\nWorkloads: ping, set, get-hit, get-miss, del-hit, mixed\n"
              << "Options:\n"
              << "  --host <IPv4 address>  Default: 127.0.0.1\n"
              << "  --port <port>          Default: 6337 for AttoKV, 6379 for Redis\n"
              << "  --clients <count>      Default: 1\n"
              << "  --requests <count>     Default: 100000\n"
              << "  --warmup <count>       Default: 10000\n"
              << "  --keyspace <count>     Default: 10000\n"
              << "  --value-size <bytes>   Default: 256\n"
              << "  --get-ratio <0-100>    Mixed workload GET percentage; default: 80\n"
              << "  --seed <number>        Default: 1\n";
}

template <typename T> bool parse_number(std::string_view input, T& output) {
    const char* end = input.data() + input.size();
    auto result = std::from_chars(input.data(), end, output);
    return result.ec == std::errc{} && result.ptr == end;
}

std::optional<Workload> parse_workload(std::string_view input) {
    if (input == "ping")
        return Workload::ping;
    if (input == "set")
        return Workload::set;
    if (input == "get-hit")
        return Workload::get_hit;
    if (input == "get-miss")
        return Workload::get_miss;
    if (input == "del-hit")
        return Workload::del_hit;
    if (input == "mixed")
        return Workload::mixed;
    return std::nullopt;
}

std::string_view workload_name(Workload workload) {
    switch (workload) {
    case Workload::ping:
        return "ping";
    case Workload::set:
        return "set";
    case Workload::get_hit:
        return "get-hit";
    case Workload::get_miss:
        return "get-miss";
    case Workload::del_hit:
        return "del-hit";
    case Workload::mixed:
        return "mixed";
    }
    return "unknown";
}

std::expected<bool, std::string> parse_options(int argc, const char** argv, Options& options) {
    for (int i = 1; i < argc; i++) {
        std::string_view argument{argv[i]};
        if (argument == "--help")
            return true;
        if (i + 1 >= argc)
            return std::unexpected{std::format("Missing value for {}", argument)};
        std::string_view value{argv[++i]};

        if (argument == "--backend") {
            if (value == "attokv")
                options.backend = BackendKind::attokv;
            else if (value == "redis")
                options.backend = BackendKind::redis;
            else
                return std::unexpected{std::format("Invalid backend: {}", value)};
        } else if (argument == "--workload") {
            options.workload = parse_workload(value);
            if (!options.workload)
                return std::unexpected{std::format("Invalid workload: {}", value)};
        } else if (argument == "--host") {
            options.host = value;
        } else if (argument == "--port") {
            if (!parse_number(value, options.port) || options.port < 1 || options.port > 65535)
                return std::unexpected{std::format("Invalid port: {}", value)};
            options.port_set = true;
        } else if (argument == "--clients") {
            if (!parse_number(value, options.clients) || options.clients == 0)
                return std::unexpected{std::format("Invalid client count: {}", value)};
        } else if (argument == "--requests") {
            if (!parse_number(value, options.requests) || options.requests == 0)
                return std::unexpected{std::format("Invalid request count: {}", value)};
        } else if (argument == "--warmup") {
            if (!parse_number(value, options.warmup))
                return std::unexpected{std::format("Invalid warmup count: {}", value)};
        } else if (argument == "--keyspace") {
            if (!parse_number(value, options.keyspace) || options.keyspace == 0)
                return std::unexpected{std::format("Invalid keyspace: {}", value)};
        } else if (argument == "--value-size") {
            if (!parse_number(value, options.value_size) || options.value_size == 0)
                return std::unexpected{std::format("Invalid value size: {}", value)};
        } else if (argument == "--get-ratio") {
            if (!parse_number(value, options.get_ratio) || options.get_ratio > 100)
                return std::unexpected{std::format("Invalid GET ratio: {}", value)};
        } else if (argument == "--seed") {
            if (!parse_number(value, options.seed))
                return std::unexpected{std::format("Invalid seed: {}", value)};
        } else {
            return std::unexpected{std::format("Unknown option: {}", argument)};
        }
    }

    if (!options.backend)
        return std::unexpected{"--backend is required"};
    if (!options.workload)
        return std::unexpected{"--workload is required"};
    if (!options.port_set)
        options.port = *options.backend == BackendKind::attokv ? 6337 : 6379;
    return false;
}

std::vector<Operation> generate_operations(const Options& options, std::size_t count,
                                           std::uint64_t seed, std::string_view phase) {
    std::mt19937_64 random{seed};
    std::vector<Operation> operations{};
    operations.reserve(count);
    for (std::size_t i = 0; i < count; i++) {
        std::size_t key_number = random() % options.keyspace;
        switch (*options.workload) {
        case Workload::ping:
            operations.push_back({.kind = OperationKind::ping});
            break;
        case Workload::set:
            operations.push_back(
                  {.kind = OperationKind::set, .key = std::format("key:{}", key_number)});
            break;
        case Workload::get_hit:
            operations.push_back(
                  {.kind = OperationKind::get_hit, .key = std::format("key:{}", key_number)});
            break;
        case Workload::get_miss:
            operations.push_back(
                  {.kind = OperationKind::get_miss, .key = std::format("missing:{}", key_number)});
            break;
        case Workload::del_hit:
            operations.push_back(
                  {.kind = OperationKind::remove, .key = std::format("delete:{}:{}", phase, i)});
            break;
        case Workload::mixed:
            operations.push_back({
                .kind = random() % 100 < options.get_ratio ? OperationKind::get_hit
                                                           : OperationKind::set,
                .key = std::format("key:{}", key_number),
            });
            break;
        }
    }
    return operations;
}

std::expected<void, std::string> execute(Backend& backend, const Operation& operation,
                                         std::string_view value) {
    switch (operation.kind) {
    case OperationKind::ping:
        return backend.ping();
    case OperationKind::set:
        return backend.set(operation.key, value);
    case OperationKind::get_hit: {
        auto result = backend.get(operation.key);
        if (!result)
            return std::unexpected{std::move(result.error())};
        if (!*result)
            return std::unexpected{std::format("Expected key to exist: {}", operation.key)};
        if (**result != value)
            return std::unexpected{std::format("Unexpected value for key: {}", operation.key)};
        return {};
    }
    case OperationKind::get_miss: {
        auto result = backend.get(operation.key);
        if (!result)
            return std::unexpected{std::move(result.error())};
        if (*result)
            return std::unexpected{std::format("Expected key to be missing: {}", operation.key)};
        return {};
    }
    case OperationKind::remove: {
        auto result = backend.remove(operation.key);
        if (!result)
            return std::unexpected{std::move(result.error())};
        if (!*result)
            return std::unexpected{std::format("Expected key to be removed: {}", operation.key)};
        return {};
    }
    }
    return std::unexpected{"Unknown operation"};
}

std::expected<void, std::string> prepare(Backend& backend, const Options& options,
                                         const std::vector<Operation>& operations,
                                         std::string_view value) {
    auto flush_result = backend.flush();
    if (!flush_result)
        return flush_result;

    if (*options.workload == Workload::get_hit || *options.workload == Workload::mixed) {
        for (std::size_t i = 0; i < options.keyspace; i++) {
            auto result = backend.set(std::format("key:{}", i), value);
            if (!result)
                return result;
        }
    } else if (*options.workload == Workload::del_hit) {
        for (const Operation& operation : operations) {
            auto result = backend.set(operation.key, value);
            if (!result)
                return result;
        }
    }
    return {};
}

PhaseResult run_phase(std::vector<std::unique_ptr<Backend>>& backends,
                      const std::vector<Operation>& operations, std::string_view value,
                      bool record_latency) {
    const std::size_t clients = backends.size();
    std::barrier ready{static_cast<std::ptrdiff_t>(clients + 1)};
    std::barrier start{static_cast<std::ptrdiff_t>(clients + 1)};
    std::barrier done{static_cast<std::ptrdiff_t>(clients + 1)};
    std::vector<std::vector<std::uint64_t>> worker_latencies(clients);
    std::vector<std::string> errors(clients);
    std::vector<std::thread> workers{};
    workers.reserve(clients);

    std::size_t offset{};
    for (std::size_t worker = 0; worker < clients; worker++) {
        std::size_t count = operations.size() / clients + (worker < operations.size() % clients);
        std::size_t begin = offset;
        offset += count;
        if (record_latency)
            worker_latencies[worker].reserve(count);

        workers.emplace_back([&, worker, begin, count] {
            ready.arrive_and_wait();
            start.arrive_and_wait();
            for (std::size_t i = begin; i < begin + count; i++) {
                auto operation_start = std::chrono::steady_clock::now();
                auto result = execute(*backends[worker], operations[i], value);
                auto operation_end = std::chrono::steady_clock::now();
                if (!result) {
                    errors[worker] = std::move(result.error());
                    break;
                }
                if (record_latency) {
                    worker_latencies[worker].push_back(static_cast<std::uint64_t>(
                          std::chrono::duration_cast<std::chrono::nanoseconds>(operation_end -
                                                                               operation_start)
                                .count()));
                }
            }
            done.arrive_and_wait();
        });
    }

    ready.arrive_and_wait();
    auto phase_start = std::chrono::steady_clock::now();
    start.arrive_and_wait();
    done.arrive_and_wait();
    auto phase_end = std::chrono::steady_clock::now();
    for (std::thread& worker : workers)
        worker.join();

    PhaseResult result{
        .elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(phase_end - phase_start),
    };
    for (std::size_t worker = 0; worker < clients; worker++) {
        if (!errors[worker].empty() && result.error.empty())
            result.error = std::format("Client {}: {}", worker, errors[worker]);
        result.latencies.insert(result.latencies.end(), worker_latencies[worker].begin(),
                                worker_latencies[worker].end());
    }
    return result;
}

std::expected<std::vector<std::unique_ptr<Backend>>, std::string>
connect_backends(const Options& options) {
    std::vector<std::unique_ptr<Backend>> backends{};
    backends.reserve(options.clients);
    for (std::size_t i = 0; i < options.clients; i++) {
        auto backend = make_backend(*options.backend);
        auto result = backend->connect(options.host, options.port);
        if (!result)
            return std::unexpected{std::format("Failed to connect client {}: {}", i,
                                               result.error())};
        backends.push_back(std::move(backend));
    }
    return backends;
}

std::uint64_t percentile(const std::vector<std::uint64_t>& values, unsigned percent) {
    std::size_t index = (values.size() * percent + 99) / 100 - 1;
    return values[index];
}

void print_result(const Options& options, const PhaseResult& result) {
    double seconds = std::chrono::duration<double>(result.elapsed).count();
    double operations_per_second = static_cast<double>(result.latencies.size()) / seconds;
    auto microseconds = [](std::uint64_t nanoseconds) {
        return static_cast<double>(nanoseconds) / 1'000.0;
    };

    std::cout << "Backend: " << (*options.backend == BackendKind::attokv ? "attokv" : "redis")
              << '\n'
              << "Endpoint: " << options.host << ':' << options.port << '\n'
              << "Workload: " << workload_name(*options.workload) << '\n'
              << "Clients: " << options.clients << '\n'
              << "Requests: " << result.latencies.size() << '\n'
              << "Warmup requests: " << options.warmup << '\n'
              << "Keyspace: " << options.keyspace << '\n'
              << "Value size: " << options.value_size << " bytes\n"
              << "GET ratio: " << options.get_ratio << "%\n"
              << "Seed: " << options.seed << '\n'
              << std::format("Elapsed: {:.6f} s\n", seconds)
              << std::format("Throughput: {:.2f} operations/s\n", operations_per_second)
              << std::format("Latency: p50={:.3f} us p95={:.3f} us p99={:.3f} us max={:.3f} us\n",
                             microseconds(percentile(result.latencies, 50)),
                             microseconds(percentile(result.latencies, 95)),
                             microseconds(percentile(result.latencies, 99)),
                             microseconds(result.latencies.back()));
}

} // namespace

int main(int argc, const char** argv) {
    Options options{};
    auto parse_result = parse_options(argc, argv, options);
    if (!parse_result) {
        std::cerr << parse_result.error() << '\n';
        print_usage();
        return 1;
    }
    if (*parse_result) {
        print_usage();
        return 0;
    }

    std::string value(options.value_size, 'x');
    auto warmup_operations = generate_operations(options, options.warmup,
                                                 options.seed ^ 0x9e3779b97f4a7c15ULL, "warmup");
    auto operations = generate_operations(options, options.requests, options.seed, "measured");

    auto backends = connect_backends(options);
    if (!backends) {
        std::cerr << backends.error() << '\n';
        return 1;
    }
    auto control = make_backend(*options.backend);
    auto connect_result = control->connect(options.host, options.port);
    if (!connect_result) {
        std::cerr << "Failed to connect control client: " << connect_result.error() << '\n';
        return 1;
    }

    if (!warmup_operations.empty()) {
        auto prepare_result = prepare(*control, options, warmup_operations, value);
        if (!prepare_result) {
            std::cerr << "Failed to prepare warmup: " << prepare_result.error() << '\n';
            return 1;
        }
        PhaseResult warmup = run_phase(*backends, warmup_operations, value, false);
        if (!warmup.error.empty()) {
            std::cerr << "Warmup failed: " << warmup.error << '\n';
            return 1;
        }
    }

    auto prepare_result = prepare(*control, options, operations, value);
    if (!prepare_result) {
        std::cerr << "Failed to prepare benchmark: " << prepare_result.error() << '\n';
        return 1;
    }
    PhaseResult result = run_phase(*backends, operations, value, true);
    if (!result.error.empty()) {
        std::cerr << "Benchmark failed: " << result.error << '\n';
        return 1;
    }
    if (result.latencies.size() != options.requests) {
        std::cerr << "Benchmark completed an unexpected number of requests\n";
        return 1;
    }

    std::sort(result.latencies.begin(), result.latencies.end());
    print_result(options, result);
    return 0;
}
