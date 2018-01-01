build depends "../deps/hash/hash.c";
export {
#include "../deps/hash/hash.h"
#include <stdlib.h>
}
#include <stdio.h>

import stream from "../../stream/stream.module.c";

export typedef struct {
  hash_t   * deps;
  hash_t   * exports;
  hash_t   * symbols;
  void    ** ordered;
  size_t     n_exports;
  char     * name;
  char     * source_abs;
  char     * source_rel;
  char     * generated;
  char     * header_abs;
  stream.t * out;
} package_t as t;

export extern hash_t * path_cache;
export extern hash_t * id_cache;
hash_t * path_cache = NULL;
hash_t * id_cache = NULL;

/* TODO: fix the circrular dependency issue */
export extern package_t * (*new)(const char * relative_path, char ** error);
package_t * (*package_new)(const char * relative_path, char ** error) = NULL;

export void emit(package_t * pkg, char * value) {
  if (pkg->out) stream.write(pkg->out, value, strlen(value));
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
