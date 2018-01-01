import stream      from "../stream/stream.module.c";

build depends "./deps/hash/hash.c";
#include "deps/hash/hash.h"

#include <stdio.h>
#include <stdarg.h>

static void * parse_c       (parser.t * p);
static void * parse_import  (parser.t * p);
static void * parse_export  (parser.t * p);
static void * parse_build   (parser.t * p);
static void * parse_package (parser.t * p);
static void * parse_id      (parser.t * p, lex_item.t item);

export void parse(stream.t * input) {
  lex.t * lexer = shell_lex.new(input);
  EVIL
  parser.parse(lexer, parse_c);
  stream.t * s = NULL;
}


static hash_t * keywords = NULL;
static void init_keywords(){
  keywords = hash_new();

  hash_set(keywords, "package", parse_package);
  hash_set(keywords, "import",  parse_import );
  hash_set(keywords, "export",  parse_export );
  hash_set(keywords, "build",   parse_build  );
  parse(NULL);
}

static void * parse_c(parser.t * p) {
  lex_item.t item;
  do {
    item = parser.next(p);

    switch(item.type) {
      case item_id:
        return parse_id(p, item);

      case item_eof:
      case item_error:
        printf("\n%s\n", lex_item.to_string(item));
        break;

      case item_c_code:
      default:
        printf("%s", item.value);
    }

  } while (item.type != item_eof && item.type != item_error);
  return NULL;
}

static void * parse_id(struct parser.t * p, lex_item.t item) {
  if (keywords == NULL) init_keywords();
  void * fn = hash_get(keywords, item.value);
  if (fn != NULL)  return fn;

  printf("%s", item.value);
  return parse_c;
}

export struct a {
  bool b;
  struct {
    char a;
  } c;
};

export enum test{
  a = 0,
  b,
  c,
  d,
  e,
  f,
  g,
} as test2;

export typedef struct a {
  bool b;
  struct {
    char a;
  } c;
} c as b;

export
union a {
  float a;
  long  b;
  int   c;
};

export typedef void * (*callback)(void);

export const long long int * test_fn(void) {
  // body
}
