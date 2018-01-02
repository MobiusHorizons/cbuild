# dependencies for ./test.c
./test.o: ./test.c ../deps/hash/hash.c ../package/index.h ../package/package.h ../package/export.h ./string-stream.h ../deps/stream/stream.h

# dependencies for ../deps/hash/hash.c
../deps/hash/hash.o: ../deps/hash/hash.c 

# dependencies for ../package/index.c
../package/index.o: ../package/index.c ../deps/hash/hash.c ../package/package.h ../parser/grammer.h ../parser/parser.h ../package/import.h ../package/export.h ../deps/stream/stream.h ../package/atomic-stream.h ../deps/stream/file.h ../package/package.h ../deps/stream/stream.h

# dependencies for ../package/package.c
../package/package.o: ../package/package.c ../deps/hash/hash.c ../deps/stream/stream.h ../deps/stream/stream.h

# dependencies for ../deps/stream/stream.c
../deps/stream/stream.o: ../deps/stream/stream.c

# dependencies for ../parser/grammer.c
../parser/grammer.o: ../parser/grammer.c ../deps/hash/hash.c ../deps/stream/stream.h ../lexer/item.h ../lexer/lex.h ../lexer/syntax.h ../package/package.h ../parser/parser.h ../parser/package.h ../parser/import.h ../parser/export.h ../parser/build.h ../parser/identifier.h ../deps/stream/stream.h ../package/package.h

# dependencies for ../lexer/item.c
../lexer/item.o: ../lexer/item.c

# dependencies for ../lexer/lex.c
../lexer/lex.o: ../lexer/lex.c ../deps/stream/stream.h ../lexer/item.h ../lexer/buffer.h ../deps/stream/stream.h ../lexer/buffer.h ../lexer/item.h

# dependencies for ../lexer/buffer.c
../lexer/buffer.o: ../lexer/buffer.c ../lexer/item.h ../lexer/item.h

# dependencies for ../lexer/syntax.c
../lexer/syntax.o: ../lexer/syntax.c ../lexer/lex.h ../deps/stream/stream.h ../lexer/lex.h ../deps/stream/stream.h

# dependencies for ../parser/parser.c
../parser/parser.o: ../parser/parser.c ../lexer/item.h ../lexer/lex.h ../lexer/stack.h ../package/package.h ../lexer/lex.h ../lexer/stack.h ../package/package.h ../lexer/item.h

# dependencies for ../lexer/stack.c
../lexer/stack.o: ../lexer/stack.c ../lexer/item.h ../lexer/item.h

# dependencies for ../parser/package.c
../parser/package.o: ../parser/package.c ../parser/parser.h ../lexer/item.h ../parser/string.h ../parser/parser.h

# dependencies for ../parser/string.c
../parser/string.o: ../parser/string.c

# dependencies for ../parser/import.c
../parser/import.o: ../parser/import.c ../parser/parser.h ../parser/string.h ../lexer/item.h ../package/import.h ../package/package.h ../utils/utils.h ../parser/parser.h

# dependencies for ../package/import.c
../package/import.o: ../package/import.c ../package/package.h ../package/export.h ../deps/hash/hash.c ../package/package.h

# dependencies for ../package/export.c
../package/export.o: ../package/export.c ../package/package.h ../deps/stream/stream.h ../package/atomic-stream.h ../utils/utils.h ../deps/hash/hash.c ../package/package.h

# dependencies for ../package/atomic-stream.c
../package/atomic-stream.o: ../package/atomic-stream.c ../deps/stream/stream.h ../deps/stream/stream.h

# dependencies for ../utils/utils.c
../utils/utils.o: ../utils/utils.c

# dependencies for ../parser/export.c
../parser/export.o: ../parser/export.c ../deps/hash/hash.c ../parser/parser.h ../parser/string.h ../utils/utils.h ../parser/identifier.h ../lexer/item.h ../package/package.h ../package/export.h ../package/import.h ../parser/parser.h

# dependencies for ../parser/identifier.c
../parser/identifier.o: ../parser/identifier.c ../deps/hash/hash.c ../lexer/item.h ../lexer/stack.h ../parser/parser.h ../package/package.h ../package/export.h ../package/import.h ../lexer/item.h ../parser/parser.h

# dependencies for ../parser/build.c
../parser/build.o: ../parser/build.c ../parser/parser.h ../parser/string.h ../lexer/item.h ../package/package.h ../package/import.h ../deps/hash/hash.c ../parser/parser.h

# dependencies for ../deps/stream/file.c
../deps/stream/file.o: ../deps/stream/file.c ../deps/stream/stream.h ../deps/stream/stream.h

# dependencies for ./string-stream.c
./string-stream.o: ./string-stream.c ../deps/stream/stream.h ../deps/stream/stream.h

test: ../deps/hash/hash.o ../deps/stream/stream.o ../package/package.o ../lexer/item.o ../lexer/buffer.o ../lexer/lex.o ../lexer/syntax.o ../lexer/stack.o ../parser/parser.o ../parser/string.o ../parser/package.o ../package/atomic-stream.o ../utils/utils.o ../package/export.o ../package/import.o ../parser/import.o ../parser/identifier.o ../parser/export.o ../parser/build.o ../parser/grammer.o ../deps/stream/file.o ../package/index.o ./string-stream.o ./test.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS)  ../deps/hash/hash.o ../deps/stream/stream.o ../package/package.o ../lexer/item.o ../lexer/buffer.o ../lexer/lex.o ../lexer/syntax.o ../lexer/stack.o ../parser/parser.o ../parser/string.o ../parser/package.o ../package/atomic-stream.o ../utils/utils.o ../package/export.o ../package/import.o ../parser/import.o ../parser/identifier.o ../parser/export.o ../parser/build.o ../parser/grammer.o ../deps/stream/file.o ../package/index.o ./string-stream.o ./test.o -o test

CLEAN_test:
	rm -rf test  ../deps/hash/hash.o ../deps/stream/stream.o ../package/package.o ../lexer/item.o ../lexer/buffer.o ../lexer/lex.o ../lexer/syntax.o ../lexer/stack.o ../parser/parser.o ../parser/string.o ../parser/package.o ../package/atomic-stream.o ../utils/utils.o ../package/export.o ../package/import.o ../parser/import.o ../parser/identifier.o ../parser/export.o ../parser/build.o ../parser/grammer.o ../deps/stream/file.o ../package/index.o ./string-stream.o ./test.o
