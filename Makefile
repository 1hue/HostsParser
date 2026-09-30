OS := $(shell uname -s)
HOSTS = /etc/hosts

ifeq ($(OS),Darwin)
CC = clang
RELEASE_FLAGS = -arch arm64 -arch x86_64
PLATFORM = macos-universal
COPY = sudo cp $(OUT) $(HOSTS).new && sudo mv $(HOSTS).new $(HOSTS) && { sudo dscacheutil -flushcache; sudo killall -HUP mDNSResponder || true; }
else
CC = gcc
PLATFORM = linux-$(shell uname -m)
COPY = sudo cp $(OUT) $(HOSTS).new && sudo mv -Z $(HOSTS).new $(HOSTS)
endif

CFLAGS = -std=gnu23 -Wall -Wextra -fsanitize=address -fsanitize=undefined -g
RELEASE_CFLAGS = -std=gnu23 -Wall -Wextra -O2 $(RELEASE_FLAGS)

BUILD = build
BIN = $(BUILD)/hostp
IN = $(BUILD)/in.hosts
OUT = $(BUILD)/hosts
URL = $(strip $(shell cat url.txt))
VERSION ?= dev
DIST = hostp-$(VERSION)-$(PLATFORM)

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
	curl -f --connect-timeout 10 --max-time 300 -D - -o $(IN) "$(URL)"

copy:
	$(COPY)

clean:
	rm -rf $(BUILD)

release:
	mkdir -p $(BUILD)/$(DIST)
	$(CC) $(RELEASE_CFLAGS) -Dmain=hosts_main -c -o $(BUILD)/hosts.o hosts.c
	$(CC) $(RELEASE_CFLAGS) -o $(BUILD)/$(DIST)/hostp bundle.c $(BUILD)/hosts.o
	cp url.txt custom.txt whitelist.txt README.md $(BUILD)/$(DIST)/
	tar -czf $(BUILD)/$(DIST).tar.gz -C $(BUILD) $(DIST)

.PHONY: all offline build fetch copy clean release
