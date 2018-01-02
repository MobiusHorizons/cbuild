# cbuild 2.0 - builder and preprocessor for modular c files  [![Build Status](https://travis-ci.org/MobiusHorizons/cbuild.svg?branch=master)](https://travis-ci.org/MobiusHorizons/cbuild)

This project provides a build system for building programs and libraries in a modular `C` syntax.
The syntax provides the following conveniences:

* Namespaces for symbols in the module
* Automatic header generation.
* Automatic Makefile generation

See [syntax.md](./syntax.md) for module syntax documentation.

# Version 2
Version 2.0 of the project represents a complete, from scratch rewrite of the project using the modular `C` syntax. The project is self-hosted meaning it is used to compile itself, but the generated `C` source files are left in the repository so that it can be build with a simple `make`.

Version 2.0 brings a number of enhancements over the original.
* Unit Tests - Not nearly everything is covered yet, but they exist and get built by Travis

* Better Lexer - Instead of using `flex` as the previous version did, this project uses a hand-coded lexer build after the style of Rob Pike's go template lexer.

* Real Parser - The first version just abused the lexer's features to determine syntax it would try to parse. As a result, many syntax errors would be pass through without any feedback, and get caught by the `C` compiler. The new hand coded parser is much better at catching syntax errors, and can provide useful error messages.

* Error Messages - Useful error messages for incorrect syntax.


# Cloning Instructions:
The project employs submodules so to clone run:

```sh
git clone --recursive https://github.com/MobiusHorizons/cbuild.git
```

# Build Instructions:
The project probably works on all `posix` based systems. It has been tested on `OSX`, `Linux`, and `FreeBSD`.

## Dependencies:
* `C` compiler.  # `clang` and `gcc` have been tested
* `make`         # Tested with GNU Make and BSD Make

to build, simply run:
```sh
make
sudo make install # if you want
```

this will generate the `cbuild` binary.

# Usage:

`cbuild <module>`

the `module` is the source for either an executable or library you would like to turn into a c project.
`cbuild` generates a `.c` and `.h` file for each module in the depencency tree of `module`. Furthermore it generates a
`.mk` file which specifies the depencencies between all the generated files and their respective objects, it also
contains a rule to build either a static library or an executable for modules where the name is `main`.
