

#include "../utils/strings.h"


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
	size_t         index;
} lex_item_t;

#ifdef MEM_DEBUG
static lex_item_t cache[64000];
static size_t item_index;
#endif


const lex_item_t lex_item_empty = {0};

lex_item_t lex_item_new(char * value, enum lex_item_type type, size_t line, size_t line_pos, size_t start) {
	lex_item_t i = {
		.value    = value,
		.length   = strings_len(value),
		.type     = type,
		.line     = line,
		.line_pos = line_pos,
		.start    = start,
#ifdef MEM_DEBUG
		.index    = ++item_index,
#endif
	};
#ifdef MEM_DEBUG
	cache[i.index - 1] = i;
#endif
	return i;
}

char * lex_item_to_string(lex_item_t item) {
	char * buf = NULL;
	const char * name = lex_item_type_names[item.type];

	switch(item.type) {
		case item_error:
			return strings_dup(item.value == NULL ? "(null)" : item.value);

		case item_eof:
			return strings_dup("<eof>");

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
	return lex_item_new(
		strings_dup(a.value),
		a.type,
		a.line,
		a.line_pos,
		a.start
	);
}

void lex_item_free(lex_item_t item) {
#ifdef MEM_DEBUG
	if (item.index > 0) {
		lex_item_t orig = cache[item.index - 1];
		if (orig.value != item.value) {
			printf("Error freeing lex item: value was '%s'. now is ", orig.value);

			printf("\n{\n"
				"    value    : '%s',\n"
				"    length   : %ld,\n"
				"    type     : '%s',\n"
				"    line     : %ld,\n"
				"    line_pos : %ld,\n"
				"    start    : %ld,\n"
				"}\n\n",
				item.value,
				item.length,
				lex_item_type_names[item.type],
				item.line,
				item.line_pos,
				item.start
			);
		} else {
			cache[item.index - 1].index = 0;
		}
	}
#endif
	free(item.value);
}

lex_item_t lex_item_replace_value(lex_item_t a, char * value) {
	lex_item_free(a);
	return lex_item_new(
		value,
		a.type,
		a.line,
		a.line_pos,
		a.start
	);
}


// Test function for debugging memory leaks
void lex_item_unfreed() {
#ifdef MEM_DEBUG
	size_t count = 0;
	size_t i;
	for (i = 0; i <= item_index; i++) {
		lex_item_t item = cache[i];
		if (item.index) {
			count++;
			printf("\n[%ld]{\n"
				"    value    : '%s',\n"
				"    length   : '%ld',\n"
				"    type     : '%s',\n"
				"    line     : '%ld',\n"
				"    line_pos : '%ld',\n"
				"    start    : '%ld',\n"
				"}\n\n",
				item.index,
				item.value,
				item.length,
				lex_item_type_names[item.type],
				item.line,
				item.line_pos,
				item.start
			);
		}

		// intentionally lose the value so valgrind reports correctly
		cache[i].value = NULL; 
	}
	if (count > 0) {
		printf("lex_item audit: found %ld unfreed items\n", count);
	}
	item_index = 0;
#endif
}