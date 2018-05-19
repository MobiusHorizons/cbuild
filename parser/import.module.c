#include <stdarg.h>
#include <stdio.h>
#include <string.h>

import parser     from "./parser.module.c";
import string     from "./string.module.c";
import lex_item   from "../lexer/item.module.c";
import pkg_import from "../package/import.module.c";
import Package    from "../package/package.module.c";
import utils      from "../utils/utils.module.c";
import str        from "../utils/strings.module.c";

static int errorf(parser.t * p, lex_item.t item, const char * fmt, ...) {
	va_list args;
	va_start(args, fmt);

	parser.verrorf(p, item, "Invalid import syntax: ", fmt, args);

	return -1;
}

export int parse(parser.t * p) {
	lex_item.t alias = parser.skip(p, item_whitespace, 0);
	if (alias.type != item_id) {
		return errorf(p, alias, "Expecting identifier, but got %s", lex_item.to_string(alias));
	}

	lex_item.t from = parser.skip(p, item_whitespace, 0);
	if (from.type != item_id || strcmp(from.value, "from") != 0) {
		return errorf(p, from, "Expecting 'from', but got %s", lex_item.to_string(from));
	}
	lex_item.free(from);

	lex_item.t filename = parser.skip(p, item_whitespace, 0);
	if (filename.type != item_quoted_string) {
		return errorf(p, filename, "Expecting filename, but got %s", lex_item.to_string(filename));
	}

	lex_item.t semicolon = parser.skip(p, item_whitespace, 0);
	if (semicolon.type != item_symbol && semicolon.value[0] != ';') {
		return errorf(p, semicolon, "Expecting ';', but got %s", lex_item.to_string(semicolon));
	}
	lex_item.free(semicolon);

	char * error = NULL;
	pkg_import.t * imp = pkg_import.add(
		str.dup(alias.value),
		str.dup(string.parse(filename.value)),
		p->pkg,
		&error
	);
	lex_item.free(alias);
	lex_item.free(filename);
	if (imp == NULL) return -1;
	if (error != NULL) return errorf(p, filename, error);

	char * include;
	char * rel = utils.relative(p->pkg->source_abs, imp->pkg->header);
	asprintf(&include, "#include \"%s\"", rel);
	Package.emit(p->pkg, include);
	global.free(include);
	global.free(rel);

	return 1;
}
