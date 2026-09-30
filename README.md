# Hosts Parser `.c`

## Requirements

- `gcc`
- `libasan` and `libubsan` (or remove the `-fsanitize` flags from `Makefile`)

## Usage

```sh
make fetch
make run
```

Or run manually:

```sh
make
./hosts-parser -i ./hosts -o ./out/hosts
```

## Commands

| Command | Description |
|---|---|
| `make` | Build `hosts-parser` binary |
| `make run` | Parse `./hosts` into `./out/hosts` |
| `make copy` | Install output to `/etc/hosts` |
| `make fetch` | Download StevenBlack blocklist |
| `make clean` | Remove built binary |

`make copy` overwrites `/etc/hosts` and likely needs `sudo`.