#ifndef _package_parser_identifier_
#define _package_parser_identifier_

#include <stdbool.h>

#include "../lexer/item.h"
#include "parser.h"

lex_item_t parser_identifier_parse_typed(parser_t * p, lex_item_t type, lex_item_t item, bool is_export);
lex_item_t parser_identifier_parse (parser_t * p, lex_item_t item, bool is_export);

#endif
