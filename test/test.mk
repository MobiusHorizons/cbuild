#dependencies for package 'test.c'
test.o: test.c ../deps/stream/stream.h string-stream.h ../package/index.h ../package/package.h

#dependencies for package '../deps/stream/stream.c'
../deps/stream/stream.o: ../deps/stream/stream.c

#dependencies for package 'string-stream.c'
string-stream.o: string-stream.c ../deps/stream/stream.h

#dependencies for package '../package/index.c'
../package/index.o: ../package/index.c ../deps/stream/stream.h ../parser/grammer.h ../package/export.h ../package/import.h ../package/package.h ../package/atomic-stream.h ../deps/stream/file.h ../parser/parser.h

#dependencies for package '../parser/grammer.c'
../parser/grammer.o: ../parser/grammer.c ../deps/stream/stream.h ../parser/parser.h ../lexer/item.h ../lexer/syntax.h ../parser/import.h ../package/package.h ../parser/export.h ../parser/package.h ../parser/identifier.h ../parser/build.h ../lexer/lex.h

#dependencies for package '../parser/parser.c'
../parser/parser.o: ../parser/parser.c ../lexer/stack.h ../lexer/item.h ../package/package.h ../lexer/lex.h

#dependencies for package '../lexer/stack.c'
../lexer/stack.o: ../lexer/stack.c ../lexer/item.h

#dependencies for package '../lexer/item.c'
../lexer/item.o: ../lexer/item.c

#dependencies for package '../package/package.c'
../package/package.o: ../package/package.c ../deps/stream/stream.h

#dependencies for package '../deps/hash/hash.c'
../deps/hash/hash.o: ../deps/hash/hash.c

#dependencies for package '../lexer/lex.c'
../lexer/lex.o: ../lexer/lex.c ../deps/stream/stream.h ../lexer/buffer.h ../lexer/item.h

#dependencies for package '../lexer/buffer.c'
../lexer/buffer.o: ../lexer/buffer.c ../lexer/item.h

#dependencies for package '../lexer/syntax.c'
../lexer/syntax.o: ../lexer/syntax.c ../lexer/lex.h ../deps/stream/stream.h

#dependencies for package '../parser/import.c'
../parser/import.o: ../parser/import.c ../package/import.h ../parser/string.h ../utils/utils.h ../lexer/item.h ../package/package.h ../parser/parser.h

#dependencies for package '../package/import.c'
../package/import.o: ../package/import.c ../package/package.h ../package/export.h

#dependencies for package '../package/export.c'
../package/export.o: ../package/export.c ../deps/stream/stream.h ../utils/utils.h ../package/atomic-stream.h ../package/package.h

#dependencies for package '../utils/utils.c'
../utils/utils.o: ../utils/utils.c

#dependencies for package '../package/atomic-stream.c'
../package/atomic-stream.o: ../package/atomic-stream.c ../deps/stream/stream.h

#dependencies for package '../parser/string.c'
../parser/string.o: ../parser/string.c

#dependencies for package '../parser/export.c'
../parser/export.o: ../parser/export.c ../parser/identifier.h ../package/export.h ../lexer/item.h ../package/package.h ../parser/parser.h

#dependencies for package '../parser/identifier.c'
../parser/identifier.o: ../parser/identifier.c ../lexer/item.h ../package/package.h ../package/export.h ../lexer/stack.h ../package/import.h ../parser/parser.h

#dependencies for package '../parser/package.c'
../parser/package.o: ../parser/package.c ../lexer/item.h ../parser/string.h ../parser/parser.h

#dependencies for package '../parser/build.c'
../parser/build.o: ../parser/build.c ../package/import.h ../parser/string.h ../lexer/item.h ../parser/parser.h

#dependencies for package '../deps/stream/file.c'
../deps/stream/file.o: ../deps/stream/file.c ../deps/stream/stream.h

test: test.o ../deps/stream/stream.o string-stream.o ../package/index.o ../parser/grammer.o ../parser/parser.o ../lexer/stack.o ../lexer/item.o ../package/package.o ../deps/hash/hash.o ../lexer/lex.o ../lexer/buffer.o ../lexer/syntax.o ../parser/import.o ../package/import.o ../package/export.o ../utils/utils.o ../package/atomic-stream.o ../parser/string.o ../parser/export.o ../parser/identifier.o ../parser/package.o ../parser/build.o ../deps/stream/file.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS) test.o ../deps/stream/stream.o string-stream.o ../package/index.o ../parser/grammer.o ../parser/parser.o ../lexer/stack.o ../lexer/item.o ../package/package.o ../deps/hash/hash.o ../lexer/lex.o ../lexer/buffer.o ../lexer/syntax.o ../parser/import.o ../package/import.o ../package/export.o ../utils/utils.o ../package/atomic-stream.o ../parser/string.o ../parser/export.o ../parser/identifier.o ../parser/package.o ../parser/build.o ../deps/stream/file.o -o test

CLEAN_test:
	rm -rf test test.o ../deps/stream/stream.o string-stream.o ../package/index.o ../parser/grammer.o ../parser/parser.o ../lexer/stack.o ../lexer/item.o ../package/package.o ../deps/hash/hash.o ../lexer/lex.o ../lexer/buffer.o ../lexer/syntax.o ../parser/import.o ../package/import.o ../package/export.o ../utils/utils.o ../package/atomic-stream.o ../parser/string.o ../parser/export.o ../parser/identifier.o ../parser/package.o ../parser/build.o ../deps/stream/file.o
