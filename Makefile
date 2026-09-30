CC = gcc
CFLAGS = -Wall -Wextra -fsanitize=address -fsanitize=undefined -g

BUILD = build
BIN = $(BUILD)/hosts-parser
OUT = $(BUILD)/hosts

all: run

$(BIN): hosts.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -o $(BIN) hosts.c

run: $(BIN) fetch
	./$(BIN) -i ./hosts -o $(OUT)
	ls -l $(OUT) /etc/hosts

fetch:
	curl -D - -O https://raw.githubusercontent.com/StevenBlack/hosts/master/alternates/fakenews-gambling-social/hosts

copy:
	sudo cp -Z $(OUT) /etc/hosts

clean:
	rm -rf $(BUILD)

.PHONY: all run fetch copy clean
