CC = gcc
CFLAGS = -Wall -Wextra -fsanitize=address -fsanitize=undefined -g

hosts-parser: hosts.c
	$(CC) $(CFLAGS) -o hosts-parser hosts.c

run: hosts-parser
	./hosts-parser -i ./data/hosts -o ./data/hosts.out
	ls -l data/hosts.out /etc/hosts

clean:
	rm -f ./hosts-parser

.PHONY: clean
