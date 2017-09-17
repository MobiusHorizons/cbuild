CFLAGS := -g $(CFLAGS) -Ideps
PREFIX  ?= /usr/local

ALL : mpp

test : mpp
	./mpp test/test.module.c

test : lexer.c

mpp-test : mpp test/test.c test/list.c 
	./mpp test/test.c | $(CC) -o ./mpp-test -x c -
	./mpp-test

mpp : main.c $(DEPS_SRC) lexer.o module.o module.h relative.o relative.h
	$(CC) -o mpp $(CFLAGS) main.c $(DEPS_SRC) module.o lexer.o relative.o
	#./mpp test/test.c > /dev/null

%.o : %.c %.h Makefile
	$(CC) $(CFLAGS) -c $< -o $@

lexer.c:	lexer.l
	flex --header-file=lexer.h -t lexer.l > lexer.c

lexer.o: lexer.c
	$(CC) $(CFLAGS) -c lexer.c -o lexer.o

clean :
	rm -rf *.o mpp lexer.c lexer.h *.re

watch : 
	while [ true ]; do \
		make test        ;\
		fswatch -1 -r . >/dev/null;\
	done

install: mpp
	cp mpp $(PREFIX)/bin
