# --- Platform ---

OS := $(shell uname -s)
HOSTS = /etc/hosts

ifeq ($(OS),Darwin)
CC = clang
PLATFORM = macos-universal
RELEASE_FLAGS = -arch arm64 -arch x86_64
else
CC = gcc
PLATFORM = linux-$(shell uname -m)
endif

# --- Build settings ---

WARN = -std=gnu23 -Wall -Wextra
CFLAGS = $(WARN) -fsanitize=address -fsanitize=undefined -g
RELEASE_CFLAGS = $(WARN) -O2 $(RELEASE_FLAGS)
CURL_FLAGS = -f --connect-timeout 10 --max-time 300 -D -

# --- Paths ---

BUILD = build
BIN = $(BUILD)/hostp
IN = $(BUILD)/in.hosts
OUT = $(BUILD)/hosts
URL = $(strip $(shell cat url.txt))
COPY_SH = copy.sh
VERSION ?= dev
DIST = hostp-$(VERSION)-$(PLATFORM)

# --- Targets ---

.PHONY: all offline build fetch copy clean release

all: fetch offline

offline: build
	"$(BIN)" -i $(IN) -o $(OUT)
	ls -l $(OUT) $(HOSTS)

build: $(if $(BUNDLED),,$(BIN))

$(BIN): hosts.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -o $(BIN) hosts.c

fetch:
	mkdir -p $(BUILD)
	curl $(CURL_FLAGS) -o $(IN) "$(URL)"

# Confirm, then replace HOSTS (see copy.sh; embedded in release builds)
copy:
	@sh "$(COPY_SH)" $(OUT) $(HOSTS)

clean:
	rm -rf $(BUILD)

release:
	mkdir -p $(BUILD)/$(DIST)
	$(CC) $(RELEASE_CFLAGS) -Dmain=hosts_main -c -o $(BUILD)/hosts.o hosts.c
	$(CC) $(RELEASE_CFLAGS) -o $(BUILD)/$(DIST)/hostp bundle.c $(BUILD)/hosts.o
	cp url.txt custom.txt whitelist.txt README.md $(BUILD)/$(DIST)/
	tar -czf $(BUILD)/$(DIST).tar.gz -C $(BUILD) $(DIST)
