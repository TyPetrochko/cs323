CC = gcc
CFLAGS = -std=c99 -Wall -pedantic -g


lzw: lzw.c StringTable.c
	$(CC) $(CFLAGS) -o $@ /c/cs323/Hwk4/code.c  $^
	rm -f encode decode
	ln lzw encode
	ln lzw decode

encode: lzw
	rm -f encode
	ln lzw encode

decode: lzw
	rm -f decode
	ln lzw decode

clean:
	rm -f lzw

testfile:
	/c/cs323/Hwk4/encode -m 9 -p 9223372036854775807 < $(FILE) | /c/cs323/Hwk4/decode | cmp - $(FILE)
	./encode -m 9 -p 9223372036854775807 < $(FILE) | ./decode | cmp - $(FILE)

tester: test.c
	$(CC) $(CFLAGS) -o stroltest $^

StringTable: StringTable.c
	$(CC) $(CFLAGS) -o $@ $^

test: lzw
	/c/cs323/bin/testit 4 lzw

submit: lzw
	/c/cs323/bin/submit 4 Makefile lzw.c lzw.h StringTable.c StringTable.h log.lzw
	/c/cs323/bin/testit 4 lzw

