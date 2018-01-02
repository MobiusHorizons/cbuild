package "parser_identifier";

build depends "../deps/hash/hash.c";
#include "../deps/hash/hash.h"
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
export {
#include <stdbool.h>
}

import lex_item   from "../lexer/item.module.c";
import stack      from "../lexer/stack.module.c";
import parser     from "./parser.module.c";
import Package    from "../package/package.module.c";
import pkg_export from "../package/export.module.c";
import pkg_import from "../package/import.module.c";

hash_t * options = NULL;
static void init_options(){
  options = hash_new();

  hash_set(options, "enum",   (void *) 1);
  hash_set(options, "union",  (void *) 2);
  hash_set(options, "struct", (void *) 3);
}

static lex_item.t token(parser.t * p, stack.t * s) {
  lex_item.t item = parser.next(p);
  while(item.type == item_whitespace){
    stack.push(s, item);
    item = parser.next(p);
  }
  return item;
}

static lex_item.t cleanup(parser.t * p, stack.t * s) {
  while(s->length > 1) {
    parser.backup(p, stack.pop(s));
  }
  lex_item.t out = stack.pop(s);
  stack.free(s);
  return out;
}

static void rewind_until(parser.t * p, stack.t * s, lex_item.t item) {
  lex_item.t i;
  while(s->length > 0 && (i = stack.pop(s)).type != 0 && !lex_item.equals(i, item)) {
    parser.backup(p, i);
  }
  stack.free(s);
}

static lex_item.t emit(parser.t * p, stack.t * s) {
  int i;
  for (i = 0; i < s->length -1; i++) {
    Package.emit(p->pkg, s->items[i].value);
  }

  lex_item.t out = s->items[s->length-1];

  stack.free(s);
  return out;
}


static lex_item.t parse_type(parser.t * p, stack.t * s, lex_item.t item, lex_item.t *type) {
  if (options == NULL) init_options();
  if (!hash_has(options, item.value)) return item;

  *type = item;
  stack.push(s, item);
  return token(p, s);
}

static lex_item.t parse_import( parser.t *p, stack.t * s,
    lex_item.t item, lex_item.t *pkg, lex_item.t *name ) {
  lex_item.t temp;

  stack.push(s, item);
  if (item.type != item_id) return lex_item.empty;
  temp = item;

  item = parser.next(p);
  stack.push(s, item);

  if (item.type != item_symbol || item.value[0] != '.') return temp;

  item = parser.next(p);
  stack.push(s, item);
  if (item.type != item_id) return lex_item.empty;

  *pkg  = temp;
  *name = item;
  return item;
}

static lex_item.t parse_symbol(parser.t *p, stack.t * s, lex_item.t type, lex_item.t item) {
  pkg_export.t * symbol = (pkg_export.t *) hash_get(p->pkg->symbols, item.value);
  if (symbol == NULL) return cleanup(p, s);

  bool has_type = type.type != 0;

  rewind_until(p, s, item);
  /*item.length = strlen(symbol->symbol);*/
  /*item.value  = symbol->symbol;*/
  item.length = asprintf(&item.value, "%s%s%s",
      has_type ? type.value : "", has_type ? " " : "", symbol->symbol);
  return item;
}

export lex_item.t parse_typed(parser.t * p, lex_item.t type, lex_item.t item, bool is_export) {
  lex_item.t ident = {0};
  lex_item.t from  = {0};
  lex_item.t name  = {0};
  stack.t * s      = stack.new(8);

  item = parse_import(p, s, item, &from, &name);
  if (item.type == 0) return cleanup(p, s);
  if (name.type == 0) return parse_symbol(p, s, lex_item.empty, item);

  if (strcmp(from.value, "global") == 0) return name;

  pkg_import.t * imp = (pkg_import.t *) hash_get(p->pkg->deps, from.value);
  if (imp == NULL || imp->pkg == NULL) return emit(p, s);

  pkg_export.t * exp = (pkg_export.t *) hash_get(imp->pkg->exports, name.value);
  if (exp == NULL) {
    parser.errorf(p, name, "", "Package '%s' does not export the symbol '%s'",
        from.value, name.value
    );
    return cleanup(p, s);
  }

  if (is_export) pkg_export.export_headers(p->pkg, imp->pkg);

  ident        = type;
  ident.length = asprintf(&ident.value, "%s %s", type.value, exp->symbol);

  return ident;
}

export lex_item.t parse (parser.t * p, lex_item.t item, bool is_export) {
  lex_item.t ident = {0};
  lex_item.t type  = {0};
  lex_item.t from  = {0};
  lex_item.t name  = {0};
  stack.t * s      = stack.new(8);


  item = parse_type(p, s, item, &type);
  item = parse_import(p, s, item, &from, &name);
  if (item.type == 0) return cleanup(p, s);
  if (name.type == 0) return parse_symbol(p, s, type, item);

  if (strcmp(from.value, "global") == 0) return name;

  pkg_import.t * imp = (pkg_import.t *) hash_get(p->pkg->deps, from.value);
  if (imp == NULL || imp->pkg == NULL) return emit(p, s);

  pkg_export.t * exp = (pkg_export.t *) hash_get(imp->pkg->exports, name.value);
  if (exp == NULL) {
    parser.errorf(p, name, "", "Package '%s' does not export the symbol '%s'",
        from.value, name.value
    );
    return cleanup(p, s);
  }

  bool has_type = type.type != 0;
  if (is_export) pkg_export.export_headers(p->pkg, imp->pkg);

  ident        = (has_type) ? type : from;
  ident.length = asprintf(&ident.value, "%s%s%s",
      has_type ? type.value : "", has_type ? " " : "", exp->symbol);
  return ident;
}


