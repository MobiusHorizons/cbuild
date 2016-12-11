# Modular C

This project is currently extremely alpha quality software.
Everything is a work in progress.

This project is an attempt to create a preprocessor for `C` that
adds some features for easier modularity. These include:

* [x] Automatic namespacing for imports
* [ ] Automatic header generation (currently no headers, just injects the file)
* More as I think of them...

# Syntax:

The preprocessor recognizes the following syntaxes:

## Imports
> Import a `c` file without using a header

### Syntax:

```javascript
import a from "./awesome.c";
```

### Description
All the exported functions from `awesome.c` are now available
under the namespace `a`. So if the function `void do(void);` is
exported from `awesome.c`, after the above import, it can be used
as `a.do();`

### Generated Syntax

`import a from "./awesome.c";` -> the contents of `awesome.c`   
`a.do();` -> `awesome_do();`   



## Exports
> Export a symbol from your module

### Syntax:
```javascript
export do;
void do(void){
  ...
}
```

### Description
The `export` declaration must occur before the symbol is defined
in the `c` file. Running `export` on a symbol prefixes it with
the module's namespace prefix (currently based on the filename).

### Generated Syntax

`void do(void){` -> `void awesome_do(void{`


# Build Instructions:

The project probably works on all `posix` based systems, however
it has only been tested on OSX.

## Dependencies:
* `flex`
* `libc`
* `C` compiler.
* `make`

to build, simply run:
```sh
make
```

this will generate the `mpp` binary.

# Usage:

At this point `mpp` takes a file as input, and outputs to
`stdout`. To compile a file `test.c` with `mpp` and `gcc`, run
the following:

```sh
mpp test.c | gcc -x c - -o test
```
