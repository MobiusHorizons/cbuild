package "package_import";

#include <stdlib.h>
#include "../deps/hash/hash.h"

import Package    from "./package.module.c";
import pkg_export from "./export.module.c";
build  depends         "../deps/hash/hash.c";

export typedef struct {
  char      * alias;
  char      * filename;
  Package.t * pkg;
} Import_t as t;

export Import_t * add(char * alias, char * filename, Package.t * parent, char ** error) {
  Import_t * imp = malloc(sizeof(Import_t));

  hash_set(parent->deps, alias, imp);

  imp->alias    = alias;
  imp->filename = filename;
  imp->pkg      = Package.new(filename, error);

  if (imp->pkg == NULL) return NULL;

  pkg_export.write_headers(imp->pkg);
  return imp;
}
