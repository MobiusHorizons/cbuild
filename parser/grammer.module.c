build depends "../deps/hash/hash.c";
#include "../deps/hash/hash.h"

#include <stdio.h>
#include <stdarg.h>

import stream     from "../deps/stream/stream.module.c";

import lex_item   from "../lexer/item.module.c";
import lex        from "../lexer/lex.module.c";
import syntax     from "../lexer/syntax.module.c";
import Package    from "../package/package.module.c";
import parser     from "./parser.module.c";

import ParsePackage from "./package.module.c";
import Import       from "./import.module.c";
import Export       from "./export.module.c";
import Build        from "./build.module.c";
import Identifier   from "./identifier.module.c";

typedef int (*keyword_fn)(parser.t * p);

static void * parse_id      (parser.t * p, lex_item.t item);
static void * parse_keyword (parser.t * p, lex_item.t item);

static void * parse_c(parser.t * p) {
	lex_item.t item = {0};
	lex_item.t last = {0};
	int escaped_id = 0;
	do {
		lex_item.free(last);
		last = item;
		item = parser.next(p);

		switch(item.type) {
			case item_arrow:
				escaped_id = 1;
				Package.emit(p->pkg, item.value);
				continue;

			case item_eof:
				break;
			case item_error:
				parser.errorf(p, item, "", "");
				lex_item.free(last);
				lex_item.free(item);
				return NULL;

			case item_id:
				if (last.type == 0 || last.start < last.line_pos) {
					lex_item.free(last);
					return parse_keyword(p, item);
				}
				if (escaped_id == 0) {
					lex_item.free(last);
					return parse_id(p, item);
				}
			case item_c_code:
			default:
				Package.emit(p->pkg, item.value);
		}

		escaped_id = 0;
	} while (item.type != item_eof && item.type != item_error);

	lex_item.free(item);
	lex_item.free(last);
	return NULL;
}

static hash_t * keywords = NULL;
static void init_keywords(){
	keywords = hash_new();

	hash_set(keywords, "package", ParsePackage.parse);
	hash_set(keywords, "import",  Import.parse );
	hash_set(keywords, "export",  Export.parse );
	hash_set(keywords, "build",   Build.parse  );
}

static void * parse_keyword(parser.t * p, lex_item.t item) {
	if (keywords == NULL) init_keywords();
	keyword_fn fn = (keyword_fn) hash_get(keywords, item.value);

	if (fn != NULL) {
		lex_item.free(item);
		if (fn(p)) return parse_c;
		return NULL;
	}

	return parse_id(p, item);
}

static void * parse_id(parser.t * p, lex_item.t item) {
	item = Identifier.parse(p, item, false);
	Package.emit(p->pkg, item.value);
	lex_item.free(item);
	return parse_c;
}

export int parse(stream.t * in, const char * filename, Package.t * p, char ** error) {
	lex.t * lexer = syntax.new(in, filename, error);
	if (lexer == NULL) return -1;

	return parser.parse(lexer, parse_c, p);
}
