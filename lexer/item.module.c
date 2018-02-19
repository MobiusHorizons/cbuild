package "lex_item";

export {
#include <stdlib.h>
#include <stdbool.h>
}
#include <stdio.h>
#include <string.h>

export enum item_type {
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
} as type;

export extern const char ** type_names;
const char * type_names[item_total_symbols] = {
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

export typedef struct {
	enum item_type type;
	char          * value;
	size_t         length;
	size_t         line;
	size_t         line_pos;
	size_t         start;
} item_t as t;

export extern const item_t empty;
const item_t empty = {0};

export item_t new(char * value, enum item_type type, size_t line, size_t line_pos, size_t start) {
	item_t i = {
		.value    = value,
		.length   = strlen(value),
		.type     = type,
		.line     = line,
		.line_pos = line_pos,
		.start    = start,
	};
	return i;
}

export char * to_string(item_t item) {
	char * buf = NULL;
	const char * name = type_names[item.type];

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

export bool equals(item_t a, item_t b) {
	return (
			a.type     == b.type     &&
			a.value    == b.value    &&
			a.length   == b.length   &&
			a.line     == b.line     &&
			a.line_pos == b.line_pos &&
			a.start    == b.start
	);
}

export item_t dup(item_t a) {
	item_t i = {
		.value    = strdup(a.value),
		.length   = strlen(a.value),
		.type     = a.type,
		.line     = a.line,
		.line_pos = a.line_pos,
		.start    = a.start,
	};
	return i;
}

export void free(item_t item) {
	global.free(item.value);
}
