DEPS_SRC := 
DEPS_INC := $(shell ls -d deps/*/ | sed -e 's/^\s*/-I/')

CCFLAGS := $(CCFLAGS) $(DEPS_INC)

ALL : mpp

test : mpp-test

mpp-test : mpp test/test.c test/list.c 
	./mpp test/test.c | $(CC) -o ./mpp-test -x c -
	./mpp-test

mpp : main.c $(DEPS_SRC) lexer.o module.o module.h
	$(CC) -o mpp $(CCFLAGS) main.c $(DEPS_SRC) module.o lexer.o

%.o : %.c %.h Makefile
	$(CC) $(CCFLAGS) -c $< -o $@

lexer.c:	lexer.l
	flex --header-file=lexer.h -t lexer.l > lexer.c

lexer.o: lexer.c
	$(CC) $(CCFLAGS) -c lexer.c -o lexer.o

clean :
	rm -rf *.o mpp lexer.c lexer.h
