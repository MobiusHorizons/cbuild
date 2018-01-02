#include <stdarg.h>
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>

#include "parser.h"
#include "string.h"
#include "../lexer/item.h"
#include "../package/import.h"
#include "../package/package.h"
#include "../utils/utils.h"

static int errorf(parser_t * p, lex_item_t item, const char * fmt, ...) {
  va_list args;
  va_start(args, fmt);

  parser_verrorf(p, item, "Invalid import syntax: ", fmt, args);

  return -1;
}

int import_parse(parser_t * p) {
  lex_item_t alias = parser_skip(p, item_whitespace, 0);
  if (alias.type != item_id) {
    return errorf(p, alias, "Expecting identifier, but got %s", lex_item_to_string(alias));
  }

  lex_item_t from = parser_skip(p, item_whitespace, 0);
  if (from.type != item_id || strcmp(from.value, "from") != 0) {
    return errorf(p, from, "Expecting 'from', but got %s", lex_item_to_string(from));
  }

  lex_item_t filename = parser_skip(p, item_whitespace, 0);
  if (filename.type != item_quoted_string) {
    return errorf(p, filename, "Expecting filename, but got %s", lex_item_to_string(filename));
  }

  lex_item_t semicolon = parser_skip(p, item_whitespace, 0);
  if (semicolon.type != item_symbol && semicolon.value[0] != ';') {
    return errorf(p, semicolon, "Expecting ';', but got %s", lex_item_to_string(semicolon));
  }

  char * error = NULL;
  package_import_t * imp = package_import_add(alias.value, string_parse(filename.value), p->pkg, &error);
  if (imp == NULL) return -1;
  if (error != NULL) return errorf(p, filename, error);

  lex_item_t include = alias;
  char * rel = utils_relative(p->pkg->source_abs, imp->pkg->header_abs);
  include.length = asprintf(&include.value, "#include \"%s\"", rel);
  package_emit(p->pkg, include.value);

  return 1;
}