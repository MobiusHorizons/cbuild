#ifndef _package_lex_item_
#define _package_lex_item_

#include <stdlib.h>
#include <stdbool.h>

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

extern const char ** lex_item_type_names;

typedef struct {
	enum lex_item_type type;
	char          * value;
	size_t         length;
	size_t         line;
	size_t         line_pos;
	size_t         start;
} lex_item_t;

extern const lex_item_t lex_item_empty;
lex_item_t lex_item_new(char * value, enum lex_item_type type, size_t line, size_t line_pos, size_t start);
char * lex_item_to_string(lex_item_t item);
bool lex_item_equals(lex_item_t a, lex_item_t b);
lex_item_t lex_item_dup(lex_item_t a);
void lex_item_free(lex_item_t item);

#endif
