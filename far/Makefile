CC= gcc
CFLAGS=-std=c99 -Wall -pedantic

Far: Far.c Far.h
	$(CC) $(CFLAGS) -o $@ $^

debug: Far.c Far.h
	$(CC) $(CFLAGS) -DDEBUG -o $@ $^

test: Far
	/c/cs323/bin/submit 1 Far.c Far.h log.Far Makefile
	/c/cs323/bin/testit 1 Far
