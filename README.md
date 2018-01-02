# cbuild - builder and preprocessor for modular c files.

This project is currently alpha quality software.
Everything is a work in progress.

This project provides a build system for building programs and libraries in a modular `C` syntax.
The syntax provides the following conveniences:

* Namespaces for symbols in the module
* Automatic header generation.
* Automatic Makefile generation

See [syntax.md](./syntax.md) for module syntax documentation.

# Cloning Instructions:
The project employes submodules so to clone run:

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
```

this will generate the `cbuild` binary.

# Usage:

`cbuild <module>`

the `module` is the source for either an executable or library you would like to turn into a c project.
`cbuild` generates a `.c` and `.h` file for each module in the depencency tree of `module`. Furthermore it generates a
`.mk` file which specifies the depencencies between all the generated files and their respective objects, it also
contains a rule to build either a static library or an executable for modules where the name is `main`.

