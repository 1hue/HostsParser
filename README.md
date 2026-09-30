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
make build/hosts-parser
./build/hosts-parser -i ./hosts -o ./build/hosts
```

## Commands

| Command | Description             |
|---|-------------------------|
| `make` / `make all` | Build, fetch, and parse |
| `make run` | Same as `make`          |
| `make build/hosts-parser` | Build binary only       |
| `make fetch` | Download blocklist      |
| `make copy` | Copy to `/etc/hosts`    |
| `make clean` | Remove `build/`         |

Output goes to `build/hosts`. `make copy` overwrites `/etc/hosts` and runs `sudo`; back it up first.