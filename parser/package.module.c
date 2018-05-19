package "parser_package";
import parser    from "./parser.module.c";
import lex_item  from "../lexer/item.module.c";
import string     from "./string.module.c";
import str        from "../utils/strings.module.c";

#include <stdarg.h>
#include <stdio.h>

static int errorf(parser.t * p, lex_item.t item, const char * fmt, ...) {
	va_list args;
	va_start(args, fmt);

	parser.verrorf(p, item, "Invalid package syntax: ", fmt, args);
	return -1;
}

export int parse(parser.t * p) {
	lex_item.t name = parser.skip(p, item_whitespace, 0);
	if (name.type != item_quoted_string) {
		return errorf(p, name, "Expecting a quoted string, but got %s", lex_item.to_string(name));
	}

	lex_item.t semicolon = parser.skip(p, item_whitespace, 0);
	if (semicolon.type != item_symbol && semicolon.value[0] != ';') {
		return errorf(p, semicolon, "Expecting ';' got %s", lex_item.to_string(semicolon));
	}
	lex_item.free(semicolon);

	global.free(p->pkg->name);
	p->pkg->name = str.dup(string.parse(name.value));
	lex_item.free(name);
	return 1;
}
