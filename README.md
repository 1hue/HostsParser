# Hosts Parser `.c`

## Requirements

- `gcc` 15+ (C23, `#embed`)
- `make`
- `curl`
- Optional (dev builds, or remove `-fsanitize` from `Makefile`):
  - `libasan`: detects buffer overflows, use-after-free, leaks
  - `libubsan`: detects integer overflow, null derefs, etc.

## Usage

```sh
make # Output goes to `build/hosts`
make copy # CAUTION: overwrites `/etc/hosts` with sudo
```

## Commands

| Command | Description |
|---|---|
| `make` | Build, fetch, and parse (safe - does not copy) |
| `make offline` | Build and parse, skip fetch (safe - does not copy) |
| `make build` | Build binary only |
| `make fetch` | Download blocklist to `build/in.hosts` |
| `make copy` | Copy output to `/etc/hosts` |
| `make clean` | Remove `build/` |
| `make release` | Package single-executable release |

## Release

Single executable bundling `Makefile` and `hosts.c`. Requires `make` and `curl`. Commands work as subcommands:

```sh
./hostp
./hostp copy
```

## Config

- [`url.txt`](url.txt) blocklist URL to fetch
- [`custom.txt`](custom.txt) lines to prepend in hostfile (block)
- [`whitelist.txt`](whitelist.txt) regexes of lines to drop
