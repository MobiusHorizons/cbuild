#ifndef _package_package_
#define _package_package_

#include "../deps/hash/hash.h"
#include <stdlib.h>
#include <stdbool.h>

#include "../deps/stream/stream.h"

typedef struct {
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
  bool       exported;
  bool       c_file;
  stream_t * out;
} package_t;

extern hash_t * package_path_cache;
extern hash_t * package_id_cache;
extern package_t * (*package_new)(const char * relative_path, char ** error);
void package_emit(package_t * pkg, char * value);
package_t * package_c_file(char * abs_path, char ** error);
void package_free(package_t * pkg);

#endif
