# Hosts Parser `.c`

## Requirements

- `gcc`
- `libasan` and `libubsan` (or remove the `-fsanitize` flags from `Makefile`)

## Usage

```sh
make
make copy
```

Or run manually:

```sh
make build
./build/hosts-parser -i ./build/hosts.in -o ./build/hosts
```

## Commands

| Command | Description                            |
|---|----------------------------------------|
| `make` | Build, fetch, and parse                |
| `make build` | Build binary only                      |
| `make fetch` | Download blocklist to `build/in.hosts` |
| `make copy` | Copy output to `/etc/hosts`            |
| `make clean` | Remove `build/`                        |

## Config

- [`custom.txt`](custom.txt) lines to prepend in hostfile (block)
- [`whitelist.txt`](whitelist.txt): regexes of lines to drop

Output goes to `build/hosts`. `make copy` overwrites `/etc/hosts` and runs `sudo`; back it up first.