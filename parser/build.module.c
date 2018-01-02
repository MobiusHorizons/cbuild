import parser     from "./parser.module.c";
import string     from "./string.module.c";
import lex_item   from "../lexer/item.module.c";
import pkg_import from "../package/import.module.c";

build depends "../deps/hash/hash.c";
#include "../deps/hash/hash.h"

#include <stdarg.h>
#include <stdio.h>

static hash_t * options = NULL;

typedef int (*parse_fn)(parser.t * p);
static int parse_depends (parser.t * p);
static int parse_set     (parser.t * p);
static int parse_append  (parser.t * p);

static void init_options(){
  options = hash_new();

  hash_set(options, "depends", parse_depends);
  hash_set(options, "set",     parse_set    );
  hash_set(options, "append",  parse_append );
}

static int errorf(parser.t * p, lex_item.t item, const char * fmt, ...) {
  va_list args;
  va_start(args, fmt);

  parser.verrorf(p, item, "Invalid build syntax: ", fmt, args);
  return -1;
}

/**********************************************************************************************************************
 * Build options manipulate the resulting makefile.
 *
 * Parses the following syntaxes:
 * - build depends      "<filename>";          # add "<filenme>" to the dependency list for the current file
 * - build set          <variable> "<value>";  # set the value of a makefile valiable         (:=)
 * - build set default  <variable> "<value>";  # set the default value of a makefile variable (?=)
 * - build append       <variable> "<value>";  # append to a makefile variable                (+=)
 **********************************************************************************************************************/
export int parse (parser.t * p) {
  if (options == NULL) init_options();

  lex_item.t item = parser.skip(p, item_whitespace, 0);
  parse_fn fn = (parse_fn) hash_get(options, item.value);
  if (fn != NULL)  return fn(p);

  return errorf(p, item, "Expecting one of \n"
      "\t'depends', 'set', 'set default' or 'append', but got %s",
      lex_item.to_string(item)
      );
}

static int parse_depends(parser.t * p) {
  lex_item.t filename = parser.skip(p, item_whitespace, 0);
  if (filename.type != item_quoted_string) {
    return errorf(p, filename, "Expecting a filename but got '%s'", filename.value);
  }

  lex_item.t semicolon = parser.skip(p, item_whitespace, 0);
  if (semicolon.type != item_symbol && semicolon.value[0] != ';') {
    return errorf(p, semicolon, "Expecting ';' but got '%s'", lex_item.to_string(semicolon));
  }

  /*printf("build depends [%s];\n", filename.value);*/
  char * error = NULL;
  pkg_import.t * imp = pkg_import.add_c_file(p->pkg, string.parse(filename.value), &error);
  if (imp == NULL) {
    parser.errorf(p, filename, "", "Error adding dependency: %s", error);
    return -1;
  }
  return 1;
}

static int parse_set(parser.t * p) {
  bool is_default = false;
  lex_item.t name;
  do {
    name = parser.skip(p, item_whitespace, 0);

    if (name.type != item_id) {
      return errorf(p, name, "Expecting a variable name, but got %s", name.value);
    }

    if (strcmp(name.value, "default") == 0) {
      name.type = 0;
      is_default = true;
    }
  } while (name.type == 0);

  lex_item.t value = parser.skip(p, item_whitespace, 0);

  if (value.type != item_quoted_string) {
    return errorf(p, value, "Experting a quoted string, but got '%s'", value.value);
  }

  lex_item.t semicolon = parser.skip(p, item_whitespace, 0);
  if (semicolon.type != item_symbol && semicolon.value[0] != ';') {
    return errorf(p, semicolon, "Expecting ';' but got '%s'", lex_item.to_string(semicolon));
  }

  printf("build set%s [%s] [%s];\n", is_default ? " default" : "", name.value, value.value);
  return 1;
}

static int parse_append(parser.t * p) {
  lex_item.t name = parser.skip(p, item_whitespace, 0);

  if (name.type != item_id) {
    return errorf(p, name, "Expecting a variable name, but got %s", name.value);
  }

  lex_item.t value = parser.skip(p, item_whitespace, 0);

  if (value.type != item_quoted_string) {
    return errorf(p, value, "Experting a quoted string, but got '%s'", value.value);
  }

  lex_item.t semicolon = parser.skip(p, item_whitespace, 0);

  if (semicolon.type != item_symbol && semicolon.value[0] != ';') {
    return errorf(p, semicolon, "Expecting ';' but got '%s'", lex_item.to_string(semicolon));
  }

  printf("build append [%s] [%s];\n", name.value, value.value);
  return 1;
}
