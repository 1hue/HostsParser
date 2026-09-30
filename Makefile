# --- Platform ---

OS := $(shell uname -s)
HOSTS = /etc/hosts

ifeq ($(OS),Darwin)
CC = clang
PLATFORM = macos-universal
RELEASE_FLAGS = -arch arm64 -arch x86_64
COPY = sudo cp $(OUT) $(HOSTS).new \
	&& sudo mv $(HOSTS).new $(HOSTS) \
	&& { sudo dscacheutil -flushcache; sudo killall -HUP mDNSResponder || true; }
size = stat -f %z $(1)
mtime = stat -f %Sm -t '%F %R' $(1)
else
CC = gcc
PLATFORM = linux-$(shell uname -m)
COPY = sudo cp $(OUT) $(HOSTS).new \
	&& sudo mv -Z $(HOSTS).new $(HOSTS)
size = stat -c %s $(1)
mtime = date -r $(1) '+%F %R'
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

# Confirm before replacing HOSTS
copy:
	@test -s $(OUT) || { echo "$(OUT) missing or empty; run make first"; exit 1; }
	@old=$$($(call size,$(HOSTS)) 2>/dev/null || echo 0); \
	new=$$($(call size,$(OUT))); \
	diff=$$(printf '%+d' $$((new - old))); \
	\
	echo "Replace $(HOSTS)?"; \
	echo "Current: $$old bytes, modified $$($(call mtime,$(HOSTS)) 2>/dev/null)"; \
	echo "New: $$new bytes ($$diff), modified $$($(call mtime,$(OUT)))"; \
	printf 'Overwrite? [y/N] '; \
	read ans; \
	\
	case "$$ans" in \
		[yY]*) $(COPY) && echo "Replaced $(HOSTS)";; \
		*) echo "Aborted; $(HOSTS) unchanged";; \
	esac

clean:
	rm -rf $(BUILD)

release:
	mkdir -p $(BUILD)/$(DIST)
	$(CC) $(RELEASE_CFLAGS) -Dmain=hosts_main -c -o $(BUILD)/hosts.o hosts.c
	$(CC) $(RELEASE_CFLAGS) -o $(BUILD)/$(DIST)/hostp bundle.c $(BUILD)/hosts.o
	cp url.txt custom.txt whitelist.txt README.md $(BUILD)/$(DIST)/
	tar -czf $(BUILD)/$(DIST).tar.gz -C $(BUILD) $(DIST)
