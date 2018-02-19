


#include <stdlib.h>
#include <stdbool.h>

#include <stdio.h>
#include <string.h>

enum lex_item_type {
	item_error = 0,
	item_eof,
	item_whitespace,
	item_c_code,
	item_id,
	item_number,
	item_char_literal,
	item_quoted_string,
	item_preprocessor,
	item_comment,

	/* symbols */
	item_symbol,
	item_open_symbol,
	item_close_symbol,
	item_arrow,

	item_total_symbols
};


const char * lex_item_type_names[item_total_symbols] = {
	"Error",
	"eof",
	"white space",
	"c code",
	"identifier",
	"number",
	"character literal",
	"string",
	"preprocessor",
	"comment",
	"symbol",
	"open symbol",
	"close symbol",
	"arrow symbol",
};

typedef struct {
	enum lex_item_type type;
	char          * value;
	size_t         length;
	size_t         line;
	size_t         line_pos;
	size_t         start;
} lex_item_t;


const lex_item_t lex_item_empty = {0};

lex_item_t lex_item_new(char * value, enum lex_item_type type, size_t line, size_t line_pos, size_t start) {
	lex_item_t i = {
		.value    = value,
		.length   = strlen(value),
		.type     = type,
		.line     = line,
		.line_pos = line_pos,
		.start    = start,
	};
	return i;
}

char * lex_item_to_string(lex_item_t item) {
	char * buf = NULL;
	const char * name = lex_item_type_names[item.type];

	switch(item.type) {
		case item_error:
			return strdup(item.value == NULL ? "(null)" : item.value);

		case item_eof:
			return strdup("<eof>");

		default:
			if (item.length > 20 || strchr(item.value, '\n') != NULL) {
				asprintf(&buf, "%s '%.*s...'", name, 20, item.value);
				return buf;
			}

			asprintf(&buf, "%s '%s'", name, item.value);
			return buf;
	}
}

bool lex_item_equals(lex_item_t a, lex_item_t b) {
	return (
			a.type     == b.type     &&
			a.value    == b.value    &&
			a.length   == b.length   &&
			a.line     == b.line     &&
			a.line_pos == b.line_pos &&
			a.start    == b.start
	);
}

lex_item_t lex_item_dup(lex_item_t a) {
	lex_item_t i = {
		.value    = strdup(a.value),
		.length   = strlen(a.value),
		.type     = a.type,
		.line     = a.line,
		.line_pos = a.line_pos,
		.start    = a.start,
	};
	return i;
}

void lex_item_free(lex_item_t item) {
	free(item.value);
}
