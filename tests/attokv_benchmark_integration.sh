#!/bin/sh

set -eu

server="$1"
benchmark="$2"
port=16337

"$server" --host 127.0.0.1 --port "$port" >/dev/null 2>&1 &
server_pid=$!
trap 'kill "$server_pid" 2>/dev/null || true; wait "$server_pid" 2>/dev/null || true' EXIT

ready=0
for attempt in 1 2 3 4 5 6 7 8 9 10; do
    if "$benchmark" --backend attokv --host 127.0.0.1 --port "$port" --workload ping \
        --clients 1 --requests 1 --warmup 0 >/dev/null 2>&1; then
        ready=1
        break
    fi
done

if [ "$ready" -ne 1 ]; then
    echo "AttoKV server did not become ready" >&2
    exit 1
fi

for workload in ping set get-hit get-miss del-hit mixed; do
    "$benchmark" --backend attokv --host 127.0.0.1 --port "$port" --workload "$workload" \
        --clients 4 --requests 100 --warmup 10 --keyspace 25 --value-size 16 >/dev/null
done
