# AttoKV

A small simple KV store, written in C++.

Includes a server and a CLI client, which communicate over TCP using a simple binary protocol.

## Building

If you have `make` installed in your system, you can very easily build the debug version of the program.

```shell
make configure # Runs CMake configure step
make build # Builds the server and client using CMake
```

You can also build server and client separately with `make`, using make target `build_server` and `build_client` respectively.

Benchmarks should always use a Release build:

```shell
make configure_release
make build_release
```

## Running

If you built the program using `make` or CMake commands targeting the same build directory, you can run the program like so;

```shell
./build/server/attokv_server # Run the server
./build/client/attokv_client # Run the client
```

The server listens on `127.0.0.1:6337` by default and the client connects to the same endpoint.
Both accept `--host` and `--port` options; use `--help` to see their complete usage.

## Binary Protocol

4-byte header which just contains the message size, then the message which is a string.

The client>server message will be the command and its arguments (passed exactly as they are typed into the CLI).

The server>client message will be the string response for the command.

## Commands

The commands are inspired by Redis, we only have a few;

| **Command** | **arguments** | **OK response**  | **Not found response** |
| ----------- | ------------- | ---------------- | ---------------------- |
| `get`       | `key`         | value            | `NULL`                 |
| `set`       | `key value`   | `OK`             | —                      |
| `del`       | `key`         | `OK`             | `NULL`                 |
| `flush`     | —             | `OK`             | —                      |
| `exit`      | —             | close connection | —                      |

## Benchmarking

The `attokv_benchmark` executable can run the same deterministic workload against AttoKV or
Redis. It uses persistent connections with one outstanding request per connection and reports
throughput together with p50, p95, p99, and maximum latency.

Start AttoKV and run a mixed workload:

```shell
./build-release/server/attokv_server
./build-release/benchmark/attokv_benchmark \
    --backend attokv --workload mixed --clients 16 --requests 1000000
```

For a comparable Redis run, start Redis without persistence and change only the backend endpoint:

```shell
redis-server --save "" --appendonly no
./build-release/benchmark/attokv_benchmark \
    --backend redis --port 6379 --workload mixed --clients 16 --requests 1000000
```

Available workloads are `ping`, `set`, `get-hit`, `get-miss`, `del-hit`, and `mixed`. Run
`attokv_benchmark --help` for workload controls, keyspace and value sizes, warmup requests, and
the deterministic seed. Servers are intentionally managed outside the harness.

## Benchmark results

As of my last run, Redis is about 50% faster than AttoKV.

These runs use;

- Both backend and benchmark running on my own machine (AMD Ryzen 5800X).
- Redis 7.4 running in Docker.
- Mixed workload preset (`--workload mixed`).

| Backend | # requests | # clients | Throughput   | p50   | p95   | p99   |
| ------- | ---------- | --------- | ------------ | ----- | ----- | ----- |
| attokv  | 100000     | 16        | 63500 ops/s  | 226us | 350us | 408us |
| Redis   | 100000     | 16        | 101134 ops/s | 151us | 221us | 308us |

## Performance notes

The KV map uses a pre-allocated array for entry (kv pairs) storage,
and a custom arena for strings which allocates in blocks to avoid invalidating pointers in the entries.

Performance generally should be good, because most operations won't need to allocate.
For example a `set` should normally just be a hash (we use Fnv1a), string compare, and two memcpy.

The `flush` command re-allocates memory every time.

The `set` command may allocate new memory if the KV map is nearly full or if the current block in the string arena is nearly full.

The `del` command may re-allocate the entire string arena if there is lots of fragmented empty space in the string arena.

`get` should be O(1) unless the KV map capacity is not very large, which would cause hash collisions (and thus, more string comparisons).

AttoKV supports simultaneous clients through the use of non-blocking sockets and polling.

## OS compatibility

AttoKV only supports Linux, for now, as I am only using Unix syscalls. It might work on MacOS, I haven't tested it.
