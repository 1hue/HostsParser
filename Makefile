CC = gcc
CFLAGS = -Wall -Wextra -fsanitize=address -fsanitize=undefined -g

hosts-parser: hosts.c
	$(CC) $(CFLAGS) -o hosts-parser hosts.c

run: hosts-parser
	mkdir -p out
	./hosts-parser -i ./hosts -o ./out/hosts
	ls -l ./out/hosts /etc/hosts

copy:
	cp -Z ./out/hosts /etc/hosts


clean:
	rm -f ./hosts-parser

fetch:
	curl -D - -O https://raw.githubusercontent.com/StevenBlack/hosts/master/alternates/fakenews-gambling-social/hosts

.PHONY: clean
