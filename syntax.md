# Module Syntax:

The preprocessor recognizes the following syntaxes:

## Imports
> Import a modular `c` file without using a header

### Syntax:

```javascript
import a from "./awesome.module.c";
```

### Description
All the exported functions/types from `awesome.module.c` are now available
under the local namespace `a`. So if the function `void do(void);` is
exported from `awesome.module.c`, after the above import, it can be used
as `a.do();`

### Generated Syntax

`import a from "./awesome.module.c";` -> `#include "./awesome.h"`
`a.do();` -> `awesome_do();`



## Exports
> Export a symbol from your module

### Syntax:
```javascript
export void do(void){
  ...
}
```

### Description
Running `export` on a symbol prefixes it with
the module's namespace prefix. This prefix is based on the filename, or can be overwritten using the `package` command.

### Generated Syntax

`export void do(void){` -> `void awesome_do(void){`


## Build Options
> Change the generated makefile.

### Syntax:
```
/* Set variables in the Makefile */
build set VAR "VALUE"
build add CFLAGS "-g"
build set default LIBRARY_INCLUDE_DIR "/path/to/default"

/* Depend on external c files */
build depends "/path/to/some/file.c"
```

### Description
`build (set | set default | add) NAME "VALUE"` will use `:=`, `?=`, `+=` respectively to set the variables in the
generated makefile whenever the module is included in the build.

`build depends "file.c"` adds `file.c` to the dependency tree whenever the module is imported. It will be added to the
generated makefile, so you can still just build your whole project with `cbuild -m root.modul.c`

### Generated Syntax
the example above would result in the following lines being added to the generated makefile.
```make
VAR := "VALUE"
CFLAGS += "-g"
LIBRARY_INCLUDE_DIR ?= "/path/to/default"
```

`build depends` will add the `file.o` to the dependencies of the current module, so it will be included in the build of
the final executable.


