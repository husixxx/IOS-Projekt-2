CC=gcc
CFLAGS= -std=gnu99 -Wall -Wextra -Werror -pedantic -pthread
all: clean proj2
proj2: proj2.c
	$(CC) $(CFLAGS) -o proj2 proj2.c

clean:
	rm -f proj2 rm -f proj2.out