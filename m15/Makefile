CC = gcc
CFLAGS = -std=c99 -Wall -pedantic -g3

m15: m15.c
	$(CC) $(CFLAGS) -o $@ $^

debug: m15.c
	$(CC) $(CFLAGS) -DDEBUG -o debug $^

test: m15
	/c/cs323/bin/testit 2 m15
