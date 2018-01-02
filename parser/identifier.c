


#include "../deps/hash/hash.h"
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>

#include <stdbool.h>


#include "../lexer/item.h"
#include "../lexer/stack.h"
#include "parser.h"
#include "../package/package.h"
#include "../package/export.h"
#include "../package/import.h"

hash_t * options = NULL;
static void init_options(){
  options = hash_new();

  hash_set(options, "enum",   (void *) 1);
  hash_set(options, "union",  (void *) 2);
  hash_set(options, "struct", (void *) 3);
}

static lex_item_t token(parser_t * p, lex_item_stack_t * s) {
  lex_item_t item = parser_next(p);
  while(item.type == item_whitespace){
    lex_item_stack_push(s, item);
    item = parser_next(p);
  }
  return item;
}

static lex_item_t cleanup(parser_t * p, lex_item_stack_t * s) {
  while(s->length > 1) {
    parser_backup(p, lex_item_stack_pop(s));
  }
  lex_item_t out = lex_item_stack_pop(s);
  lex_item_stack_free(s);
  return out;
}

static void rewind_until(parser_t * p, lex_item_stack_t * s, lex_item_t item) {
  lex_item_t i;
  while(s->length > 0 && (i = lex_item_stack_pop(s)).type != 0 && !lex_item_equals(i, item)) {
    parser_backup(p, i);
  }
  lex_item_stack_free(s);
}

static lex_item_t emit(parser_t * p, lex_item_stack_t * s) {
  int i;
  for (i = 0; i < s->length -1; i++) {
    package_emit(p->pkg, s->items[i].value);
  }

  lex_item_t out = s->items[s->length-1];

  lex_item_stack_free(s);
  return out;
}


static lex_item_t parse_type(parser_t * p, lex_item_stack_t * s, lex_item_t item, lex_item_t *type) {
  if (options == NULL) init_options();
  if (!hash_has(options, item.value)) return item;

  *type = item;
  lex_item_stack_push(s, item);
  return token(p, s);
}

static lex_item_t parse_import( parser_t *p, lex_item_stack_t * s,
    lex_item_t item, lex_item_t *pkg, lex_item_t *name ) {
  lex_item_t temp;

  lex_item_stack_push(s, item);
  if (item.type != item_id) return lex_item_empty;
  temp = item;

  item = parser_next(p);
  lex_item_stack_push(s, item);

  if (item.type != item_symbol || item.value[0] != '.') return temp;

  item = parser_next(p);
  lex_item_stack_push(s, item);
  if (item.type != item_id) return lex_item_empty;

  *pkg  = temp;
  *name = item;
  return item;
}

static lex_item_t parse_symbol(parser_t *p, lex_item_stack_t * s, lex_item_t type, lex_item_t item) {
  package_export_t * symbol = (package_export_t *) hash_get(p->pkg->symbols, item.value);
  if (symbol == NULL) return cleanup(p, s);

  bool has_type = type.type != 0;

  rewind_until(p, s, item);
  /*item.length = strlen(symbol->symbol);*/
  /*item.value  = symbol->symbol;*/
  item.length = asprintf(&item.value, "%s%s%s",
      has_type ? type.value : "", has_type ? " " : "", symbol->symbol);
  return item;
}

lex_item_t parser_identifier_parse_typed(parser_t * p, lex_item_t type, lex_item_t item, bool is_export) {
  lex_item_t ident = {0};
  lex_item_t from  = {0};
  lex_item_t name  = {0};
  lex_item_stack_t * s      = lex_item_stack_new(8);

  item = parse_import(p, s, item, &from, &name);
  if (item.type == 0) return cleanup(p, s);
  if (name.type == 0) return parse_symbol(p, s, lex_item_empty, item);

  if (strcmp(from.value, "global") == 0) return name;

  package_import_t * imp = (package_import_t *) hash_get(p->pkg->deps, from.value);
  if (imp == NULL || imp->pkg == NULL) return emit(p, s);

  package_export_t * exp = (package_export_t *) hash_get(imp->pkg->exports, name.value);
  if (exp == NULL) {
    parser_errorf(p, name, "", "Package '%s' does not export the symbol '%s'",
        from.value, name.value
    );
    return cleanup(p, s);
  }

  if (is_export) package_export_export_headers(p->pkg, imp->pkg);

  ident        = type;
  ident.length = asprintf(&ident.value, "%s %s", type.value, exp->symbol);

  return ident;
}

lex_item_t parser_identifier_parse (parser_t * p, lex_item_t item, bool is_export) {
  lex_item_t ident = {0};
  lex_item_t type  = {0};
  lex_item_t from  = {0};
  lex_item_t name  = {0};
  lex_item_stack_t * s      = lex_item_stack_new(8);


  item = parse_type(p, s, item, &type);
  item = parse_import(p, s, item, &from, &name);
  if (item.type == 0) return cleanup(p, s);
  if (name.type == 0) return parse_symbol(p, s, type, item);

  if (strcmp(from.value, "global") == 0) return name;

  package_import_t * imp = (package_import_t *) hash_get(p->pkg->deps, from.value);
  if (imp == NULL || imp->pkg == NULL) return emit(p, s);

  package_export_t * exp = (package_export_t *) hash_get(imp->pkg->exports, name.value);
  if (exp == NULL) {
    parser_errorf(p, name, "", "Package '%s' does not export the symbol '%s'",
        from.value, name.value
    );
    return cleanup(p, s);
  }

  bool has_type = type.type != 0;
  if (is_export) package_export_export_headers(p->pkg, imp->pkg);

  ident        = (has_type) ? type : from;
  ident.length = asprintf(&ident.value, "%s%s%s",
      has_type ? type.value : "", has_type ? " " : "", exp->symbol);
  return ident;
}

