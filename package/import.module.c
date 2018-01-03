package "package_import";
#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include "../deps/hash/hash.h"
export {
#include <stdbool.h>
}

import Package    from "./package.module.c";
import pkg_export from "./export.module.c";
build  depends         "../deps/hash/hash.c";

export typedef struct {
  char      * alias;
  char      * filename;
  bool        c_file;
  Package.t * pkg;
} Import_t as t;

export Import_t * add(char * alias, char * filename, Package.t * parent, char ** error) {
  Import_t * imp = malloc(sizeof(Import_t));

  hash_set(parent->deps, alias, imp);

  imp->alias    = alias;
  imp->filename = filename;
  imp->c_file   = false;
  imp->pkg      = Package.new(filename, error, parent->force, parent->silent);

  if (imp->pkg == NULL) return NULL;

  pkg_export.write_headers(imp->pkg);
  return imp;
}

export Import_t * add_c_file(Package.t * parent, char * filename, char ** error) {
  char * alias   = realpath(filename, NULL);

  Import_t * imp = malloc(sizeof(Import_t));

  hash_set(parent->deps, alias, imp);

  imp->alias    = alias;
  imp->filename = filename;
  imp->c_file   = true;
  imp->pkg      = Package.c_file(alias, error);

  return imp;
}

export Import_t * passthrough(Package.t * parent, char * filename, char ** error) {
  Import_t * imp = add(filename, filename, parent, error);
  if (*error != NULL) return NULL;
  if (imp == NULL || imp->pkg == NULL) {
    asprintf(error, "Could not import '%s'", filename);
    return NULL;
  }

  hash_each(imp->pkg->exports, {
    hash_set(parent->exports, strdup(key), val);
  });

  pkg_export.export_headers(parent, imp->pkg);
  return imp;
}
