#ifndef _package_parser_
#define _package_parser_

#include <stdbool.h>
#include <stdarg.h>

struct parser_parser_s;

typedef void * (*parser_parse_fn)(struct parser_parser_s * lex);

#include "../lexer/lex.h"
#include "../lexer/stack.h"
#include "../package/package.h"

typedef struct parser_parser_s {
  lex_t        * lexer;
  parser_parse_fn       state;
  lex_item_stack_t      * items;
  package_t    * pkg;
  int            errors;
} parser_t;

int parser_parse(lex_t * lexer, parser_parse_fn start, package_t * pkg);

#include "../lexer/item.h"

lex_item_t parser_next(parser_t * p);
void parser_backup(parser_t *p, lex_item_t item);
lex_item_t parser_skip(parser_t * p, ...);
lex_item_t parser_collect(parser_t * p, char ** buf, ssize_t * blen, ...);
void parser_verrorf(parser_t * p, lex_item_t item, const char * context, const char * fmt, va_list args);
void parser_errorf(parser_t * p, lex_item_t item, const char * context, const char * fmt, ...);

#endif
