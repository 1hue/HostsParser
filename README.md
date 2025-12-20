# Hosts Parser `.c`

Install:

1. `gcc`

2. `libasan` and `libubsan` _or_ remove `fsanitize` params from `Makefile`

Then:

```sh
make run
```

Alternatively:

```sh
make
./hosts-parser -i ./hosts-input -o ./hosts-output
```
