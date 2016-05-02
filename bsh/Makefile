CFLAGS= -g3 -Wall -std=c99 -pedantic

HWK5 = /c/cs323/Hwk5

all:    Bsh

Bsh:    ${HWK5}/mainBsh.o process.o ${HWK5}/parse.o ${HWK5}/getLine.o
	${CC} ${CFLAGS} -o $@ $^

mainBsh.o: ${HWK5}/getLine.h ${HWK5}/parse.h ${HWK5}/process-stub.h

test: all
	/c/cs323/bin/submit 5 Makefile process.c process.h log.Bsh
	/c/cs323/bin/testit 5 Bsh

gen:
	./contextfree.py stage < grammar.json >> script
	cat script

custom:
	rm -f errorout.*
	./Bsh < script > scriptout.me 2> ierrorout.me
	final/Bsh < script > scriptout.stan 2> ierrorout.stan
	/c/cs323/Hwk5/Tests/suppress ierrorout.me >> errorout.me
	/c/cs323/Hwk5/Tests/suppress ierrorout.stan >> errorout.stan
	rm ierrorout.*
	wc -l < errorout.stan >> scriptout.stan
	wc -l < errorout.me >> scriptout.me
	cmp scriptout.*

compare:
	cat scriptout.me
	cat scriptout.stan

clean:
	rm -f *.o Bsh
