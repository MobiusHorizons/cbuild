CFLAGS := -g $(CFLAGS) -Ideps
PREFIX  ?= /usr/local

ALL : cbuild

cbuild : main.c $(DEPS_SRC) lexer.o module.o module.h relative.o relative.h
	$(CC) -o cbuild $(CFLAGS) main.c $(DEPS_SRC) module.o lexer.o relative.o

%.o : %.c %.h Makefile
	$(CC) $(CFLAGS) -c $< -o $@

lexer.c:	lexer.l
	flex --header-file=lexer.h -t lexer.l > lexer.c

lexer.o: lexer.c
	$(CC) $(CFLAGS) -c lexer.c -o lexer.o

clean :
	rm -rf *.o cbuild lexer.c lexer.h *.re

install: cbuild
	cp cbuild $(PREFIX)/bin
