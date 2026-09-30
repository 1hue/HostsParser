CC = gcc
CFLAGS = -Wall -Wextra -fsanitize=address -fsanitize=undefined -g

BUILD = build
BIN = $(BUILD)/hosts-parser
IN = $(BUILD)/in.hosts
OUT = $(BUILD)/hosts
URL = https://raw.githubusercontent.com/StevenBlack/hosts/master/alternates/fakenews-gambling-social/hosts

all: build fetch
	./$(BIN) -i $(IN) -o $(OUT)
	ls -l $(OUT) /etc/hosts

build: $(BIN)

$(BIN): hosts.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -o $(BIN) hosts.c

fetch:
	mkdir -p $(BUILD)
	curl -D - -o $(IN) $(URL)

copy:
	sudo cp -Z $(OUT) /etc/hosts

clean:
	rm -rf $(BUILD)

.PHONY: all build fetch copy clean