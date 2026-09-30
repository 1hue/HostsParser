# Hosts Parser `.c`

## Usage

Requires `make` and `curl`. Download and extract the [latest release](../../releases/latest), then:

```sh
./hostp # Output goes to `build/hosts`
./hostp copy # CAUTION: overwrites `/etc/hosts` with sudo
```

`./hostp offline` skips the download and reuses the last one.

## Config

- [`url.txt`](url.txt) blocklist URL to fetch
- [`custom.txt`](custom.txt) lines to prepend in hostfile (block)
- [`whitelist.txt`](whitelist.txt) regexes of lines to drop (allow)

## Development

### Requires:

- `gcc` 15+ (C23, `#embed`)
- `make`
- `curl`
- Optional (remove `-fsanitize` from `Makefile` to skip):
  - `libasan`: detects buffer overflows, use-after-free, leaks
  - `libubsan`: detects integer overflow, null derefs, etc.

```sh
make # Output goes to `build/hosts`
make copy # CAUTION: overwrites `/etc/hosts` with sudo
```

| Command | Description |
|---|---|
| `make` | Build, fetch, and parse (safe - does not copy) |
| `make offline` | Build and parse, skip fetch (safe - does not copy) |
| `make build` | Build binary only |
| `make fetch` | Download blocklist to `build/in.hosts` |
| `make copy` | Copy output to `/etc/hosts` |
| `make clean` | Remove `build/` |
| `make release` | Package single-executable release |
