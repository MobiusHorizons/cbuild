build depends "../deps/hash/hash.c";
export {
#include "../deps/hash/hash.h"
#include <stdlib.h>
#include <stdbool.h>
}
#include <stdio.h>

import stream from "../deps/stream/stream.module.c";

export enum var_type {
  build_var_set = 0,
  build_var_set_default,
  build_var_append,
};

export typedef struct {
  char * name;
  char * value;
  enum var_type operation;
} var_t;

export typedef struct {
  hash_t   * deps;
  hash_t   * exports;
  hash_t   * symbols;
  void    ** ordered;
  size_t     n_exports;
  var_t    * variables;
  size_t     n_variables;
  char     * name;
  char     * source_abs;
  char     * source_rel;
  char     * generated;
  char     * header_abs;
  size_t     errors;
  bool       exported;
  bool       c_file;
  bool       force;
  bool       silent;
  stream.t * out;
} package_t as t;

export extern hash_t * path_cache;
export extern hash_t * id_cache;
hash_t * path_cache = NULL;
hash_t * id_cache = NULL;

/* TODO: fix the circrular dependency issue */
export extern package_t * (*new)(const char * relative_path, char ** error, bool force, bool silent);
package_t * (*package_new)(const char * relative_path, char ** error) = NULL;

export void emit(package_t * pkg, char * value) {
  if (pkg->out) stream.write(pkg->out, value, strlen(value));
}

export package_t * c_file(char * abs_path, char ** error) {
  package_t * cached = hash_get(path_cache, abs_path);
  if (cached != NULL) return cached;

  package_t * pkg = calloc(1, sizeof(package_t));

  pkg->name       = abs_path;
  pkg->source_abs = abs_path;
  pkg->generated  = abs_path;
  pkg->c_file     = true;

  hash_set(path_cache, abs_path, pkg);
  return pkg;
}

export void free(package_t * pkg) {
  if (pkg == NULL) return;

  hash_clear(pkg->deps);
  hash_clear(pkg->exports);
  hash_clear(pkg->symbols);

  global.free(pkg->name);
  global.free(pkg->source_abs);
  global.free(pkg->source_rel);
  global.free(pkg->generated);
  global.free(pkg->header_abs);
  global.free(pkg);
}
