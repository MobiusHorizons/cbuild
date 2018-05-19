
#include "parser.h"
#include "../lexer/item.h"
#include "string.h"
#include "../utils/strings.h"

#include <stdarg.h>
#include <stdio.h>

static int errorf(parser_t * p, lex_item_t item, const char * fmt, ...) {
	va_list args;
	va_start(args, fmt);

	parser_verrorf(p, item, "Invalid package syntax: ", fmt, args);
	return -1;
}

int parser_package_parse(parser_t * p) {
	lex_item_t name = parser_skip(p, item_whitespace, 0);
	if (name.type != item_quoted_string) {
		return errorf(p, name, "Expecting a quoted string, but got %s", lex_item_to_string(name));
	}

	lex_item_t semicolon = parser_skip(p, item_whitespace, 0);
	if (semicolon.type != item_symbol && semicolon.value[0] != ';') {
		return errorf(p, semicolon, "Expecting ';' got %s", lex_item_to_string(semicolon));
	}
	lex_item_free(semicolon);

	free(p->pkg->name);
	p->pkg->name = strings_dup(string_parse(name.value));
	lex_item_free(name);
	return 1;
}