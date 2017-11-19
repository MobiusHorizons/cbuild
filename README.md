# cbuild - builder and preprocessor for modular c files.

This project is currently alpha quality software.
Everything is a work in progress.

This project provides a build system for building programs and libraries in a modular `C` syntax.
The syntax provides the following conveniences:

* Namespaces for symbols in the module
* Automatic header generation.
* Automatic Makefile generation

# Build Instructions:

The project probably works on all `posix` based systems. It has been tested on `OSX`, `Linux`, and `FreeBSD`.

## Dependencies:
* `flex`         # only required if you edit the lexer.l file
* `libc`
* `C` compiler.  # `clang` and `gcc` have been tested
* `make`         # Tested with GNU Make and BSD Make

to build, simply run:
```sh
make
```

this will generate the `cbuild` binary.

# Usage:

`cbuild [command] [options] <module>`

## Options:

* -v         verbose

## Commands:

* build      generates source files and builds the module (default)
* generate   generates source files
* clean      clean all generated sources, object files, and executables

the `module` is the source for either an executable or library you would like to turn into a c project. 
`cbuild` generates a `.c` and `.h` file for each module in the depencency tree of `module`. Furthermore it generates a
`.mk` file which specifies the depencencies between all the generated files and their respective objects, it also
contains a rule to build either a static library or an executable for modules where the name is `main`.

# Syntax:

See [syntax.md](./syntax.md)
