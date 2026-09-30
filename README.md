# Hosts Parser `.c`

## Requirements

- `gcc`
- `libasan` and `libubsan` (or remove the `-fsanitize` flags from `Makefile`)

## Usage

```sh
make # Output goes to `build/hosts`
make copy # CAUTION: overwrites `/etc/hosts` with sudo
```

## Commands

| Command | Description                                    |
|---|------------------------------------------------|
| `make` | Build, fetch, and parse (safe - does not copy) |
| `make offline` | Build and parse, skip fetch (safe - does not copy)                    |
| `make build` | Build binary only                              |
| `make fetch` | Download blocklist to `build/in.hosts`         |
| `make copy` | Copy output to `/etc/hosts`                    |
| `make clean` | Remove `build/`                                |
| `make release` | Package single-executable release              |

## Release

Single executable bundling `Makefile` and `hosts.c`. Requires `make` and `curl`. Commands work as subcommands:

```sh
./hosts-parser
./hosts-parser copy
```

## Config

- [`url.txt`](url.txt) blocklist URL to fetch
- [`custom.txt`](custom.txt) lines to prepend in hostfile (block)
- [`whitelist.txt`](whitelist.txt) regexes of lines to drop
