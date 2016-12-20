DEPS_SRC := 
DEPS_INC := $(shell ls -d deps/*/ | sed -e 's/^\s*/-I/')

CCFLAGS := -g $(CCFLAGS) $(DEPS_INC)

ALL : mpp

test : mpp
	./mpp test/test.module.c

mpp-test : mpp test/test.c test/list.c 
	./mpp test/test.c | $(CC) -o ./mpp-test -x c -
	./mpp-test

mpp : main.c $(DEPS_SRC) lexer.o module.o module.h
	$(CC) -o mpp $(CCFLAGS) main.c $(DEPS_SRC) module.o lexer.o
	#./mpp test/test.c > /dev/null

%.o : %.c %.h Makefile
	$(CC) $(CCFLAGS) -c $< -o $@

#lexer.c: lexer.re
	#re2c -o lexer.c lexer.re

#lexer.re: lexer.perplex.l
	#perplex -t perplex_template.c -i lexer.h -o lexer.re lexer.perplex.l

lexer.c:	lexer.l
	flex --header-file=lexer.h -t lexer.l > lexer.c

lexer.o: lexer.c
	$(CC) $(CCFLAGS) -c lexer.c -o lexer.o

clean :
	rm -rf *.o mpp lexer.c lexer.h *.re

watch : 
	while [ true ]; do \
		make test        ;\
		fswatch -1 -r . >/dev/null;\
	done

