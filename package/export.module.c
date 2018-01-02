package "package_export";

#include <stdlib.h>
#define _GNU_SOURCE
#include <stdio.h>
#include "../deps/hash/hash.h"
#include <stdbool.h>

import Package from "./package.module.c";
import stream  from "../deps/stream/stream.module.c";
import atomic  from "./atomic-stream.module.c";
import utils   from "../utils/utils.module.c";
build  depends      "../deps/hash/hash.c";

export enum export_type {
  type_block = 0,
  type_type,
  type_function,
  type_enum,
  type_union,
  type_struct,
  type_header,
} as type;

const char * type_names[] = {
  "block",
  "type",
  "function",
  "enum",
  "union",
  "struct",
  "header",
};

export typedef struct {
  char      * local_name;
  char      * export_name;
  char      * declaration;
  char      * symbol;
  enum export_type   type;
  Package.t * pkg;
} Export_t as t;

static hash_t * types = NULL;
static void init_types() {
  types = hash_new();

  hash_set(types, "typedef",  (void*)type_type);
  hash_set(types, "function", (void*)type_function);
  hash_set(types, "enum",     (void*)type_enum);
  hash_set(types, "union",    (void*)type_union);
  hash_set(types, "struct",   (void*)type_struct);
  hash_set(types, "header",   (void*)type_header);
}

export char * add(char * local, char * alias, char * symbol, char * type, char * declaration, Package.t * parent) {
  if (types == NULL) init_types();

  Export_t * exp = malloc(sizeof(Export_t));

  exp->local_name  = local;
  exp->export_name = alias == NULL ? local : alias;
  exp->declaration = declaration;
  exp->type        = (enum export_type) hash_get(types, type);
  exp->symbol      = symbol;

  if (!hash_has(parent->exports, exp->export_name)) {
    parent->ordered = realloc(parent->ordered, sizeof(void*) *parent->n_exports + 1);
    parent->ordered[parent->n_exports] = exp;
    parent->n_exports ++;
  }

  hash_set(parent->exports, exp->export_name, exp);
  hash_set(parent->symbols, exp->local_name,  exp);

  return exp->export_name;
}

static char * get_header_path(char * generated) {
  char * header = strdup(generated);
  size_t len = strlen(header);
  header[len - 1] = 'h';
  return header;
}

#define B(a) (a ? "true" : "false")

export void write_headers(Package.t * pkg) {
  if (pkg->header_abs) return;

  pkg->header_abs = get_header_path(pkg->generated);
  stream.t * header = atomic.open(pkg->header_abs);

  stream.printf(header, "#ifndef _package_%s_\n" "#define _package_%s_\n\n", pkg->name, pkg->name);

  enum export_type last_type;
  bool had_newline   = true;

  int i;
  for (i = 0; i < pkg->n_exports; i++){
      Export_t * exp = (Export_t *) pkg->ordered[i];
      bool newline   = false;
      bool prefix    = false;
      bool multiline = index(exp->declaration, '\n') != NULL;

      if (had_newline == false && (last_type != exp->type || multiline)) prefix = true;
      /*if (had_newline == false && multiline) prefix = true;*/
      if (multiline) newline = true;

      stream.printf(header, "%s%s\n%s", prefix ? "\n" : "", exp->declaration, newline ? "\n" : "");

      last_type     = exp->type;
      had_newline   = newline;
  }
  stream.printf(header, "%s#endif\n", had_newline ? "" : "\n");
  stream.close(header);
}

export void export_headers(Package.t * pkg, Package.t * dep) {
  if (pkg == NULL || dep == NULL) return;
  write_headers(dep);

  char * rel  = utils.relative(pkg->source_abs, dep->header_abs);
  char * decl = NULL;
  asprintf(&decl, "#include \"%s\"", rel);

  add(strdup(rel), NULL, strdup(""), "header", decl, pkg);
  free(rel);
}
