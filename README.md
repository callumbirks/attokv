# AttoKV

A small simple KV store, written in C++.

Includes a server and a CLI client, which communicate over TCP using a simple binary protocol.

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

## Performance

Haven't got round to benchmarks yet, but the KV map uses a pre-allocated array for entry (kv pairs) storage,
and a custom arena for strings which allocates in blocks to avoid invalidating pointers in the entries.

Performance generally should be good, because most operations won't need to allocate.
For example a `set` should normally just be a hash (we use Fnv1a), string compare, and two memcpy.

The `flush` command re-allocates memory every time.

The `set` command may allocate new memory if the KV map is nearly full or if the current block in the string arena is nearly full.

The `del` command may re-allocate the entire string arena if there is lots of fragmented empty space in the string arena.

`get` should be O(1) unless the KV map capacity is not very large, which would cause hash collisions (and thus, more string comparisons).

For now, we only handle 1 client at a time, until I implement polling or something along those lines.
