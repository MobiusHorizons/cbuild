#include "../lexer/item.h"
#include "../lexer/lex.h"
#include "../lexer/stack.h"
#include "../package/package.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include "./colors.h"

#include <stdbool.h>
#include <stdarg.h>
;

struct parser_parser_s;
typedef void * (*parser_parse_fn)(struct parser_parser_s * lex);

typedef struct parser_parser_s {
	lex_t        * lexer;
	parser_parse_fn       state;
	lex_item_stack_t      * items;
	package_t    * pkg;
	int            errors;
} parser_t;

int parser_parse(lex_t * lexer, parser_parse_fn start, package_t * pkg) {
	parser_t * p = malloc(sizeof(parser_t));
	p->lexer     = lexer;
	p->state     = start;
	p->items     = lex_item_stack_new(1);
	p->pkg       = pkg;
	p->errors    = 0;

	char * cwd = getcwd(NULL, 0);

	char * directory = strdup(lexer->filename);
	chdir(dirname(directory));
	free(directory);

	while (p->state != NULL) p->state = (parser_parse_fn) p->state(p);
	lex_free(lexer);
	lex_item_stack_free(p->items);

	chdir(cwd);
	free(cwd);
	int errors = p->errors;
	free(p);

	if (errors > 0) fprintf(stderr, "%d error%s generated\n", errors, errors == 1 ? "" : "s");
	return errors;
}

lex_item_t parser_next(parser_t * p) {
	lex_item_t item;
	if (p->items->length) {
		item = lex_item_stack_pop(p->items);
	} else {
		item = lex_next_item(p->lexer);
	}
	return item;
}

void parser_backup(parser_t *p, lex_item_t item) {
	p->items = lex_item_stack_push(p->items, item);
}

lex_item_t parser_skip(parser_t * p, ...) {
	va_list args;
	int len = 0;
	int i;

	enum lex_item_type types[item_total_symbols];
	va_start(args, p);

	do {
		types[len] = va_arg(args, enum lex_item_type);
		len++;
	} while (types[len-1] != 0);
	len--;

	lex_item_t item;
	do {
		int do_skip = 0;
		item = parser_next(p);
		for ( i = 0; i < len; i++) {
			if (types[i] == item.type) {
				do_skip = 1;
				break;
			};
		}
		if (do_skip) continue;
		return item;
	} while (item.type != item_eof && item.type != item_error);

	return item;
}

lex_item_t parser_collect(parser_t * p, char ** buf, ssize_t * blen, ...) {
	va_list args;
	int len = 0;
	int i;

	char * text     = buf  == NULL ? NULL : *buf;
	size_t text_len = blen == NULL ? 0    : *blen;

	enum lex_item_type types[item_total_symbols];
	va_start(args, blen);

	do {
		types[len] = va_arg(args, enum lex_item_type);
		len++;
	} while (types[len-1] != 0);
	len--;

	lex_item_t item;
	do {
		int do_skip = 0;
		item = parser_next(p);
		for ( i = 0; i < len; i++) {
			if (types[i] == item.type) {
				do_skip = 1;
				text = realloc(text, text_len + item.length + 1);
				strcpy(text + text_len, item.value);
				text_len += item.length;
				break;
			};
		}
		if (do_skip) continue;
		break;
	} while (item.type != item_eof && item.type != item_error);

	*buf  = text;
	*blen = text_len;
	return item;
}

void parser_verrorf(parser_t * p, lex_item_t item, const char * context, const char * fmt, va_list args) {
	size_t start      = item.start;
	size_t line       = item.line;
	size_t line_pos   = item.line_pos;
	int    col        = start - line_pos;

	p->errors ++;

	fprintf(stderr, BOLD "%s:%ld:%d: " RESET BOLDRED "error: " RESET BOLD "%s",
			p->lexer->filename,
			line + 1,
			col + 1,
			context
	);

	if (item.type == item_error) {
		fprintf(stderr, "%s", lex_item_to_string(item));
	} else {
		vfprintf(stderr, fmt, args);
	}

	if (p->lexer->input != NULL && p->lexer->length > line_pos) {
		char * line_start  = p->lexer->input + line_pos;
		char * line_end    = strchr(line_start, '\n');
		int    line_length = line_end == NULL ? col + item.length : line_end - line_start;

		fprintf(stderr, RESET "\n%.*s\n", line_length, line_start);
		fprintf(stderr, GREEN "%*.*s^\n" RESET, col, col, " ");
	} else {
		fprintf(stderr, RESET "\n");
	}
}

void parser_errorf(parser_t * p, lex_item_t item, const char * context, const char * fmt, ...) {
	va_list args;
	va_start(args, fmt);
	parser_verrorf(p, item, context, fmt, args);
}
