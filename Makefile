# --- Platform ---

OS := $(shell uname -s)
MAC := $(filter Darwin,$(OS))
HOSTS = /etc/hosts

ifdef MAC
CC = clang
PLATFORM = macos-universal
RELEASE_FLAGS = -arch arm64 -arch x86_64
HARDEN = -D_FORTIFY_SOURCE=3 -fstack-protector-strong
else
CC = gcc
PLATFORM = linux-$(shell uname -m)
HARDEN = -fhardened -Wno-hardened
endif

# --- Build settings ---

# WERROR=-Werror (set in CI) turns warnings into errors
WARN = -std=gnu23 -Wall -Wextra -Wformat=2 -Wshadow $(WERROR)
CFLAGS = $(WARN) -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -g
RELEASE_CFLAGS = $(WARN) -O2 $(HARDEN) -ftrivial-auto-var-init=zero $(RELEASE_FLAGS)
CURL_FLAGS = -f --proto =https --max-filesize 50M --connect-timeout 10 --max-time 300 -D -

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
.DELETE_ON_ERROR:

all: fetch offline

offline: build
	"$(BIN)" -i $(IN) -o $(OUT)
	ls -lh $(if $(MAC),-O,-Z --time-style=long-iso) $(OUT) $(HOSTS)

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
	cp *.txt README.md LICENSE $(BUILD)/$(DIST)/
	tar -czf $(BUILD)/$(DIST).tar.gz -C $(BUILD) $(DIST)
