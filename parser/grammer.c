
#include "../deps/hash/hash.h"

#include <stdio.h>
#include <stdarg.h>

#include "../deps/stream/stream.h"

#include "../lexer/item.h"
#include "../lexer/lex.h"
#include "../lexer/syntax.h"
#include "../package/package.h"
#include "parser.h"

#include "package.h"
#include "import.h"
#include "export.h"
#include "build.h"
#include "identifier.h"

typedef int (*keyword_fn)(parser_t * p);

static void * parse_id      (parser_t * p, lex_item_t item);
static void * parse_keyword (parser_t * p, lex_item_t item);

static void * parse_c(parser_t * p) {
	lex_item_t item = {0};
	lex_item_t last = {0};
	int escaped_id = 0;
	do {
	lex_item_free(last);
		last = item;
		item = parser_next(p);

		switch(item.type) {
			case item_arrow:
				escaped_id = 1;
				package_emit(p->pkg, item.value);
				continue;

			case item_eof:
				break;
			case item_error:
				parser_errorf(p, item, "", "");
		lex_item_free(last);
		lex_item_free(item);
				return NULL;

			case item_id:
				if (last.type == 0 || last.start < last.line_pos) {
			lex_item_free(last);
			return parse_keyword(p, item);
		}
				if (escaped_id == 0) {
			lex_item_free(last);
			return parse_id(p, item);
		}
			case item_c_code:
			default:
				package_emit(p->pkg, item.value);
		}

		escaped_id = 0;
	} while (item.type != item_eof && item.type != item_error);

	lex_item_free(item);
	lex_item_free(last);
	return NULL;
}

static hash_t * keywords = NULL;
static void init_keywords(){
	keywords = hash_new();

	hash_set(keywords, "package", parser_package_parse);
	hash_set(keywords, "import",  import_parse );
	hash_set(keywords, "export",  export_parse );
	hash_set(keywords, "build",   build_parse  );
}

static void * parse_keyword(parser_t * p, lex_item_t item) {
	if (keywords == NULL) init_keywords();
	keyword_fn fn = (keyword_fn) hash_get(keywords, item.value);

	if (fn != NULL) {
		lex_item_free(item);
		if (fn(p)) return parse_c;
		return NULL;
	}

	return parse_id(p, item);
}

static void * parse_id(parser_t * p, lex_item_t item) {
	item = parser_identifier_parse(p, item, false);
	package_emit(p->pkg, item.value);
	lex_item_free(item);
	return parse_c;
}

int grammer_parse(stream_t * in, const char * filename, package_t * p, char ** error) {
	lex_t * lexer = lex_syntax_new(in, filename, error);
	if (lexer == NULL) return -1;

	return parser_parse(lexer, parse_c, p);
}
