#include <stdarg.h>
#include <stdio.h>
#include <string.h>


#include "../deps/hash/hash.h"

#include "parser.h"
#include "string.h"
#include "../utils/utils.h"
#include "identifier.h"
#include "../lexer/item.h"
#include "../package/package.h"
#include "../package/export.h"
#include "../package/import.h"

const lex_item_t enum_i   = { .value = "enum",   .length = 4, .type = item_id };
const lex_item_t union_i  = { .value = "union",  .length = 5, .type = item_id };
const lex_item_t struct_i = { .value = "struct", .length = 6, .type = item_id };

typedef struct {
	lex_item_t * items;
	size_t       length;
	bool         error;
} decl_t;

static lex_item_t errorf(parser_t * p, lex_item_t item, decl_t * decl, const char * fmt, ...) {
	va_list args;
	va_start(args, fmt);

	parser_verrorf(p, item, "Invalid export syntax: ", fmt, args);
	decl->error = true;
	return lex_item_empty;
}

static void append(decl_t * decl, lex_item_t value) {
	decl->items = realloc(decl->items, sizeof(lex_item_t) * (decl->length + 1));
	decl->items[decl->length] = value;
	decl->length++;
}

static void rewind_until(parser_t * p, decl_t * decl, lex_item_t item) {
	ssize_t last = decl->length -1;

	while (last >= 0 && !lex_item_equals(item, decl->items[last])) {
		parser_backup(p, decl->items[last]);
		last--;
	}
	parser_backup(p, decl->items[last]);

	decl->length = last;
}

static void rewind_whitespace(parser_t * p, decl_t * decl, lex_item_t item) {
	parser_backup(p, item);

	ssize_t last = decl->length -1;
	while (last >= 0 && decl->items[last].type == item_whitespace) {
		parser_backup(p, decl->items[last]);
		last--;
	}

	decl->length = last + 1;
}

static lex_item_t symbol_rename(parser_t * p, decl_t * decl, lex_item_t name, lex_item_t alias) {
	char * export_name = alias.type == 0 ? name.value : alias.value;
	int i;
	for (i = 0; i < decl->length; i++) {
		lex_item_t item = decl->items[i];
		if (lex_item_equals(name, item)) {
			item.length = asprintf(&item.value, "%s_%s", p->pkg->name, export_name);
			decl->items[i] = item;
			return item;
		}
	}
	return lex_item_empty;
}

static void free_decl(decl_t * decl) {
	int i;
	/*for (i = 0; i < decl->length; i++) {*/
		/*lex_item.free(decl->items[i]);*/
	/*}*/
	free(decl->items);
}

static char * emit(parser_t * p, decl_t * decl, bool is_function, bool is_extern) {
	size_t length = 0;
	int start = 0;
	int end = decl->length;
	int i;

	// trim leading/trailing whitespace
	while (start < end && decl->items[start  ].type == item_whitespace) start++;
	while (end > start && decl->items[end - 1].type == item_whitespace) end--;

	for (i = 0; i < decl->length; i++) {
		lex_item_t item = decl->items[i];
		if (!is_extern) package_emit(p->pkg, item.value);
		if (i >= start && i < end) length += item.length;
	}

	if (is_function) length ++;

	char * output = malloc(length + 1);
	length = 0;
	for (i = start; i < end; i++) {
		lex_item_t item = decl->items[i];
		memcpy(output + length, item.value, item.length);
		length += item.length;
	}

	if (is_function && !is_extern) output[length++] = ';';

	output[length] = 0;
	free_decl(decl);
	return output;
}

static lex_item_t collect_newlines(parser_t * p, decl_t * decl) {
	size_t line     = p->lexer->line;
	lex_item_t item = parser_next(p);

	while (item.type == item_whitespace || item.type == item_comment) {
		if (p->lexer->line != line) {
			append(decl, item);
		} else {
		lex_item_free(item);
	}

		line = p->lexer->line;
		item = parser_next(p);
	}

	return item;
}

static lex_item_t collect(parser_t * p, decl_t * decl) {
	lex_item_t item = parser_next(p);

	while (item.type == item_whitespace || item.type == item_comment) {
		append(decl, item);
		item = parser_next(p);
	}

	return item;
}

static lex_item_t parse_typedef       (parser_t * p, decl_t * decl);
static lex_item_t parse_struct        (parser_t * p, decl_t * decl);
static lex_item_t parse_enum          (parser_t * p, decl_t * decl);
static lex_item_t parse_union         (parser_t * p, decl_t * decl);
static lex_item_t parse_struct_block  (parser_t * p, decl_t * decl, lex_item_t start);
static lex_item_t parse_enum_block    (parser_t * p, decl_t * decl, lex_item_t start);
static lex_item_t parse_export_block  (parser_t * p, decl_t * decl);
static lex_item_t parse_as            (parser_t * p, decl_t * decl);
static lex_item_t parse_variable      (parser_t * p, decl_t * decl);
static lex_item_t parse_function      (parser_t * p, decl_t * decl);
static lex_item_t parse_function_args (parser_t * p, decl_t * decl);

static int parse_passthrough (parser_t * p);

static hash_t * export_types = NULL;
static void init_export_types(){
	export_types = hash_new();

	hash_set(export_types, "typedef", parse_typedef);
	hash_set(export_types, "struct",  parse_struct );
	hash_set(export_types, "enum",    parse_enum   );
	hash_set(export_types, "union",   parse_union  );
}

typedef lex_item_t (*export_fn)(parser_t * p, decl_t * decl);

void parse_semicolon(parser_t * p, decl_t * decl) {
	if (decl->error) return;

	lex_item_t item = collect_newlines(p, decl);

	if (item.type != item_symbol || item.value[0] != ';') {
		errorf(p, item, decl, "expecting ';' but got %s", lex_item_to_string(item));
		return;
	}
	append(decl, item);
}

int export_parse(parser_t * p) {
	if (export_types == NULL) init_export_types();

	decl_t decl  = {0};
	export_fn fn = NULL;
	lex_item_t name  = {0};
	lex_item_t alias = {0};
	bool has_semicolon = false;
	bool is_extern     = false;
	int t = 0;

	lex_item_t type = collect_newlines(p, &decl);

	switch (type.type) {
		case item_id:
			if (strcmp("extern", type.value) == 0) {
				append(&decl, type);
				is_extern = true;
				type = collect(p, &decl);
			}
			fn = (export_fn) hash_get(export_types, type.value);
			has_semicolon = is_extern || fn != NULL;
			t = 1;
			break;
		case item_symbol:
			if (type.value[0] == '*') return parse_passthrough(p);
			break;
		case item_open_symbol:
			if (type.value[0] == '{') {
				fn = parse_export_block;
				type.value = "";
				type.length = 0;
				t = 0;
			}
			break;
		default:
			fn = NULL;
			break;
	}
	if (fn == NULL) {
		parser_backup(p, type);
		type.type = item_symbol;
		type.value = "variable";
		type.length = strlen("variable");
		name = parse_variable(p, &decl);
		t = 2;
	} else {
		if (t == 1) append(&decl, type);
		name = fn(p, &decl);
	}

	alias = parse_as(p, &decl);
	if (has_semicolon) parse_semicolon(p, &decl);
	if (decl.error) {
		free_decl(&decl);
		return -1;
	}

	if (name.type == 0) {
		errorf(p, type, &decl, "can't export anonymous symbols");
		free_decl(&decl);
		return -1;
	}

	lex_item_t symbol = symbol_rename(p, &decl, name, alias);

	if (symbol.type == 0 && fn != parse_export_block) {
		errorf(p, type, &decl, "unable to export symbol '%s'", name.value);
		free_decl(&decl);
		return -1;
	}

	package_export_add(name.value, alias.value, symbol.value, type.value, emit(p, &decl, t==2, is_extern), p->pkg);
	return 1;
}

static int parse_passthrough(parser_t * p) {
	decl_t decl = {0};
	lex_item_t from = collect(p, &decl);
	if (from.type != item_id || strcmp(from.value, "from") != 0) {
		parser_errorf(p, from, "Exporting passthrough: ", "expected 'from', but got %s", lex_item_to_string(from));
		free_decl(&decl);
		return -1;
	}

	lex_item_t filename = collect(p, &decl);
	if (filename.type != item_quoted_string) {
		parser_errorf(p, filename, "Exporting passthrough: ", "expected filename, but got %s", lex_item_to_string(filename));
		free_decl(&decl);
		return -1;
	}

	parse_semicolon(p, &decl);
	if (decl.error) {
		free_decl(&decl);
		return -1;
	}

	char * error = NULL;
	package_import_t * imp = package_import_passthrough(p->pkg, string_parse(filename.value), &error);
	if (imp == NULL) {
		parser_errorf(p, filename, "Exporting passthrough: ", "%s", error);
		free_decl(&decl);
		return -1;
	}

	char * header = NULL;
	asprintf(&header, "#include \"%s\"", utils_relative(p->pkg->source_abs, imp->pkg->header));
	package_emit(p->pkg, header);

	free_decl(&decl);
	free(header);
	return 1;
}

static lex_item_t parse_as(parser_t * p, decl_t * decl) {
	if (decl->error) return lex_item_empty;

	size_t start = decl->length;

	lex_item_t as = collect(p, decl);
	if (as.type != item_id || strcmp("as", as.value) != 0) {
		parser_backup(p, as);
		while(decl->length > start) {
			decl->length--;
			parser_backup(p, decl->items[decl->length]);
		}
		return lex_item_empty;
	}

	while(decl->length > start) {
		decl->length--;
		parser_backup(p, decl->items[decl->length]);
	}

	lex_item_t alias = collect_newlines(p, decl);

	if (alias.type != item_id) {
		errorf(p, alias, decl, "expecting identifier but got %s", lex_item_to_string(alias));
		return lex_item_empty;
	}

	return alias;
}

static lex_item_t parse_function_ptr(parser_t * p, decl_t * decl) {
	lex_item_t star = collect(p, decl);
	if (star.type != item_symbol || star.value[0] != '*') {
		return errorf(p, star, decl, "function pointer: expecting '*' but found '%s'", star.value);
	}
	append(decl, star);

	lex_item_t name = collect(p, decl);
	if (name.type != item_id) {
		return errorf(p, name, decl, "function pointer: expecting identifier but found '%s'", name.value);
	}
	append(decl, name);

	lex_item_t item;

	item = collect(p, decl);
	if (item.type != item_close_symbol || item.value[0] != ')') {
		return errorf(p, item, decl, "function pointer: expecting ')' but found '%s'", item.value);
	}
	append(decl, item);

	item = collect(p, decl);
	if (item.type != item_open_symbol || item.value[0] != '(') {
		return errorf(p, item, decl, "function pointer: expecting '(' but found '%s'", item.value);
	}
	append(decl, item);

	parse_function_args(p, decl);

	if (decl->error) return lex_item_empty;
	return name;
}
static lex_item_t parse_declaration(parser_t * p, decl_t * decl) {
	lex_item_t type = collect(p, decl);
	if (type.type != item_id) {
		if (type.type == item_close_symbol && type.value[0] == '}') return type;
		return errorf(p, type, decl, "in declaration: expecting identifier but got %s",
				lex_item_to_string(type));
	}
	if (strcmp("as", type.value) == 0) return type;
	type = parser_identifier_parse(p, type, true);

	export_fn fn = (export_fn) hash_get(export_types, type.value);

	if (fn == parse_typedef) {
		return errorf(p, type, decl, "in declaration: unexpected identifier 'typedef'");
	}

	if (fn == NULL) {
		fn = parse_variable;
	}
	append(decl, type);

	lex_item_t item = fn(p, decl);
	if (decl->error) return lex_item_empty;

	if (fn == parse_enum || fn == parse_union || fn == parse_struct) {
		item = collect(p, decl);
		if (item.type != item_id) {
			parser_backup(p, item);
		} else {
			append(decl, item);
		}
	}
	parse_semicolon(p, decl);
	if (decl->error) return lex_item_empty;
	return item;
}


static lex_item_t parse_typedef (parser_t * p, decl_t * decl) {
	lex_item_t type = collect(p, decl);
	if (type.type != item_id) {
		return errorf(p, type, decl, "in typedef: expected identifier");
	}

	export_fn fn = (export_fn) hash_get(export_types, type.value);

	/*if (fn != parse_struct && fn != parse_enum && fn != parse_union) append(decl, type);*/
	append(decl, type);

	if (fn != NULL) {
		fn(p, decl);
		if (decl->error) return lex_item_empty;
	}

	lex_item_t item;
	lex_item_t name = {0};
	bool function_ptr = false;
	bool as           = false;
	do {
		item = collect(p, decl);

		switch(item.type) {
			case item_eof:
				return errorf(p, item, decl, "in typedef: expected identifier or '('");
			case item_id:
				if (strcmp("as", item.value) == 0) {
					rewind_whitespace(p, decl, item);
					as = true;
					continue;
				}
				if (!function_ptr) name = item;
				break;
			case item_symbol:
				if (item.value[0] == ';') continue;
			case item_open_symbol:
				if (!function_ptr && item.value[0] == '(') {
					function_ptr = true;
					append(decl, item);
					name = parse_function_ptr(p, decl);
					if (decl->error) return name;
					continue;
				}
			default:
				break;
		}

		append(decl, item);
	} while (
			item.type != item_error                             &&
			!(item.type == item_symbol && item.value[0] == ';') &&
			!as
	);

	if (item.type == item_error) {
		decl->error = true;
		return item;
	}

	if (!as) parser_backup(p, item);
	return name;
}

static lex_item_t parse_struct (parser_t * p, decl_t * decl) {
	lex_item_t name = {0};
	lex_item_t item = collect(p, decl);
	if (item.type == item_id) {
		name = parser_identifier_parse_typed(p, struct_i, item, true);
		append(decl, name);
		item = collect(p, decl);
	}

	if (item.type != item_open_symbol || item.value[0] != '{') {
		parser_backup(p, item);
		return name;
	}

	append(decl, item);
	parse_struct_block(p, decl, item);

	return (decl->error) ? lex_item_empty : name;
}

static lex_item_t parse_union (parser_t * p, decl_t * decl) {
	lex_item_t name = {0};
	lex_item_t item = collect(p, decl);
	if (item.type == item_id) {
		name = parser_identifier_parse_typed(p, union_i, item, true);
		append(decl, name);
		item = collect(p, decl);
	}

	if (item.type != item_open_symbol || item.value[0] != '{') {
		return errorf(p, item, decl, "in union: expecting '{' but got %s", lex_item_to_string(item));
	}

	append(decl, item);
	parse_struct_block(p, decl, item);
	return (decl->error) ? lex_item_empty : name;
}

static lex_item_t parse_enum (parser_t * p, decl_t * decl) {
	lex_item_t name = {0};
	lex_item_t item = collect(p, decl);
	if (item.type == item_id) {
		name = parser_identifier_parse_typed(p, enum_i, item, true);
		append(decl, name);
		item = collect(p, decl);
	}

	if (item.type != item_open_symbol || item.value[0] != '{') {
		return errorf(p, item, decl, "in enum: expecting '{' but got %s", lex_item_to_string(item));
	}

	append(decl, item);
	parse_enum_block(p, decl, item);
	return (decl->error) ? lex_item_empty : name;
}

static lex_item_t parse_export_block(parser_t * p, decl_t * decl) {
	lex_item_t item;

	int escaped_id = 0;
	while(true) {
		int record = 0;
		item = collect(p, decl);

		switch(item.type) {
			case item_c_code:
			case item_quoted_string:
			case item_comment:
			case item_symbol:
			case item_preprocessor:
				record = 1;
				break;

			case item_arrow:
				escaped_id = 1;
				record     = 1;
				break;

			case item_id:
				if (escaped_id == 0) item = parser_identifier_parse(p, item, true);
				record     = 1;
				escaped_id = 0;
				break;

			case item_close_symbol:
				if (item.value[0] == '}') {
					item.value  = "block";
					item.length = strlen("block");
					return item;
				};
				break;

			case item_eof:
				return errorf(p, item, decl, "in block: unmatched '{'");
			case item_error:
				parser_backup(p, item);
				decl->error = true;
				return lex_item_empty;
			default:
				return errorf(p, item, decl, "in block: unexpected input '%s'", lex_item_to_string(item));
		}
		if (record) append(decl, item);
	}

}

static lex_item_t parse_struct_block( parser_t * p, decl_t * decl, lex_item_t start) {
	lex_item_t item;
	do {
		item = parse_declaration(p, decl);
		if (item.type == item_eof) {
				return errorf(p, start, decl, "in struct block: missing matching '}' before end of file");
		}
		if (item.type == item_error) return item;
	} while (!(item.type == item_close_symbol && item.value[0] == '}'));
	append(decl, item);
	return item;
}

static lex_item_t parse_enum_declaration(parser_t * p, decl_t * decl) {
	lex_item_t item = collect(p, decl);
	if (item.type != item_id) {
		if (item.type == item_close_symbol && item.value[0] == '}') return item;
		return errorf(p, item, decl, "in enum: expecting identifier but got %s",
				lex_item_to_string(item));
	}
	if (strcmp("as", item.value) == 0) return item;
	append(decl, item);

	item = collect(p, decl);
	if (!(item.type == item_symbol && (item.value[0] != ',' || item.value[0] != '='))) {
		return item;
	}
	append(decl, item);

	if (item.value[0] == '=') {
		item = collect(p, decl);
		if (item.type != item_number) {
			return errorf(p, item, decl, "in enum: expecting a number but got %s",
					lex_item_to_string(item));
		}
		append(decl, item);

		item = collect(p, decl);
		if (item.type != item_symbol || item.value[0] != ',') {
			return item;
		}
		append(decl, item);
	}

	return item;
}

static lex_item_t parse_enum_block ( parser_t * p, decl_t * decl, lex_item_t start) {
	lex_item_t item;
	do {
		item = parse_enum_declaration(p, decl);
		if (item.type == item_eof) {
				return errorf(p, start, decl, "in struct block: missing matching '}' before end of file");
		}
		if (item.type == item_error) return item;
	} while (!(item.type == item_close_symbol && item.value[0] == '}'));
	append(decl, item);
	return item;
}

static lex_item_t parse_type (parser_t * p, decl_t * decl) {
	lex_item_t item;
	lex_item_t name = {0};
	int escaped_id = 0;
	do {
		item = collect(p, decl);
		switch(item.type) {
			case item_arrow:
				escaped_id = 1;
				append(decl, item);
				continue;
			case item_id:
				if (escaped_id == 0) item = parser_identifier_parse(p, item, true);
				name = item;
				break;
			case item_symbol:
				if (item.value[0] == '*') break;
			case item_open_symbol:
				if (item.value[0] == '(') {
					append(decl, item);
					return name;
				}
			default:
				return errorf(p, item, decl, "in type declaration: expecting identifier or '('");
		}
		append(decl, item);
	} while (item.type != item_open_symbol || item.value[0] != '(');
	return lex_item_empty;
}

static lex_item_t parse_variable (parser_t * p, decl_t * decl) {
	lex_item_t item;
	lex_item_t name = {0};
	bool escaped_id = 0;
	bool escape_till_semicolon = false;
	do {
		item = collect(p, decl);
		if (escape_till_semicolon && (item.type != item_symbol || item.value[0] != ';')) {
			append(decl, item);
			continue;
		}
		switch(item.type) {
			case item_arrow:
				escaped_id = true;
				append(decl, item);
				continue;
			case item_id:
				if (strcmp(item.value, "as") == 0) {
					rewind_whitespace(p, decl, item);
					return name;
				}
				if (escaped_id == false) item = parser_identifier_parse(p, item, true);
				name = item;
				break;
			case item_symbol:
				if (item.value[0] == '*') break;
				if (item.value[0] == '=') {
					escape_till_semicolon = true;
					break;
				}
				if (item.value[0] == ';') {
					parser_backup(p, item);
					return name;
				}
			case item_open_symbol:
				if (item.value[0] == '(') {
					lex_item_t star = parser_next(p);
					parser_backup(p, star);
					if (star.type == item_symbol && star.value[0] == '*') {
						append(decl, item);
						return parse_function_ptr(p, decl);
					}
					parser_backup(p, item);
					rewind_until(p, decl, name);
					return parse_function(p, decl);
				}
				if (item.value[0] == '[') {
					append(decl, item);
					item = parser_next(p);
					if (item.type == item_number) {
						append(decl, item);
						item = parser_next(p);
					}
					if (item.type != item_close_symbol || item.value[0] != ']') {
						return errorf(p, item, decl,
								"in type declaration: expecting terminating ']' but got %s",
								lex_item_to_string(item)
						);
					}
					break;
				}
			default:
				return errorf(p, item, decl, "in type declaration: expecting identifier or '(' but got %s", lex_item_to_string(item));
		}
		append(decl, item);
	} while (item.type != item_eof);
	return name;
}

static lex_item_t parse_function_args(parser_t * p, decl_t * decl) {
	int level = 1;
	int escaped_id = 0;
	lex_item_t item;
	do {
		item = collect(p, decl);
		switch(item.type) {
			case item_arrow:
				escaped_id = 1;
				append(decl, item);
				continue;
			case item_id:
				if (escaped_id == 0) {
					item = parser_identifier_parse(p, item, true);
				}
				break;
			case item_open_symbol:
				if (item.value[0] == '(') level++;
				break;
			case item_close_symbol:
				if (item.value[0] == ')') level--;
				break;
			default:
				break;
		}

		escaped_id = 0;
		append(decl, item);
	} while (level > 0 && item.type != item_eof && item.type != item_error);
	return item;
}

static lex_item_t parse_function (parser_t * p, decl_t * decl) {
	lex_item_t name = parse_type(p, decl);

	if (decl->error) {
		emit(p, decl, true, false);
		decl->items = NULL;
		decl->length = 0;
		return lex_item_empty;
	}

	parse_function_args(p, decl);

	if (decl->error) return lex_item_empty;
	return name;
}