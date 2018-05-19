import lex_item from "../lexer/item.module.c";
import lex      from "../lexer/lex.module.c";
import stack    from "../lexer/stack.module.c";
import Package  from "../package/package.module.c";

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include "./colors.h"
export {
#include <stdbool.h>
#include <stdarg.h>
};

export struct parser_s;
export typedef void * (*parse_fn)(struct parser_s * lex);

export typedef struct parser_s {
	lex.t        * lexer;
	parse_fn       state;
	stack.t      * items;
	Package.t    * pkg;
	int            errors;
} parser_t as t;

export int parse(lex.t * lexer, parse_fn start, Package.t * pkg) {
	parser_t * p = malloc(sizeof(parser_t));
	p->lexer     = lexer;
	p->state     = start;
	p->items     = stack.new(1);
	p->pkg       = pkg;
	p->errors    = 0;

	char * cwd = getcwd(NULL, 0);

	char * directory = strdup(lexer->filename);
	chdir(dirname(directory));
	free(directory);

	while (p->state != NULL) p->state = (parse_fn) p->state(p);

	lex.free(lexer);
	stack.free(p->items);

	chdir(cwd);
	free(cwd);
	int errors = p->errors;
	free(p);

	if (errors > 0) fprintf(stderr, "%d error%s generated\n", errors, errors == 1 ? "" : "s");
	return errors;
}

export lex_item.t next(parser_t * p) {
	lex_item.t item;
	if (p->items->length) {
		item = stack.pop(p->items);
	} else {
		item = lex.next_item(p->lexer);
	}
	return item;
}

export void backup(parser_t *p, lex_item.t item) {
	p->items = stack.push(p->items, item);
}

export lex_item.t skip(parser_t * p, ...) {
	va_list args;
	int len = 0;
	int i;

	enum lex_item.type types[item_total_symbols];
	va_start(args, p);

	do {
		types[len] = va_arg(args, enum lex_item.type);
		len++;
	} while (types[len-1] != 0);
	len--;

	lex_item.t item = {0};
	do {
		int do_skip = 0;
		lex_item.free(item);
		item = next(p);
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

export lex_item.t collect(parser_t * p, char ** buf, ssize_t * blen, ...) {
	va_list args;
	int len = 0;
	int i;

	char * text     = buf  == NULL ? NULL : *buf;
	size_t text_len = blen == NULL ? 0    : *blen;

	enum lex_item.type types[item_total_symbols];
	va_start(args, blen);

	do {
		types[len] = va_arg(args, enum lex_item.type);
		len++;
	} while (types[len-1] != 0);
	len--;

	lex_item.t item;
	do {
		int do_skip = 0;
		item = next(p);
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

export void verrorf(parser_t * p, lex_item.t item, const char * context, const char * fmt, va_list args) {
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
		fprintf(stderr, "%s", lex_item.to_string(item));
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

export void errorf(parser_t * p, lex_item.t item, const char * context, const char * fmt, ...) {
	va_list args;
	va_start(args, fmt);
	verrorf(p, item, context, fmt, args);
}
