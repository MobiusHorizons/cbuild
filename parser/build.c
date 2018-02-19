#include "parser.h"
#include "string.h"
#include "../lexer/item.h"
#include "../package/package.h"
#include "../package/import.h"


#include "../deps/hash/hash.h"

#include <stdarg.h>
#include <stdio.h>

static hash_t * options = NULL;

typedef int (*parse_fn)(parser_t * p);
static int parse_depends (parser_t * p);
static int parse_set     (parser_t * p);
static int parse_append  (parser_t * p);

static void init_options(){
	options = hash_new();

	hash_set(options, "depends", parse_depends);
	hash_set(options, "set",     parse_set    );
	hash_set(options, "append",  parse_append );
}

static int errorf(parser_t * p, lex_item_t item, const char * fmt, ...) {
	va_list args;
	va_start(args, fmt);

	parser_verrorf(p, item, "Invalid build syntax: ", fmt, args);
	return -1;
}

/**********************************************************************************************************************
 * Build options manipulate the resulting makefile.
 *
 * Parses the following syntaxes:
 * - build depends      "<filename>";          # add "<filenme>" to the dependency list for the current file
 * - build set          <variable> "<value>";  # set the value of a makefile valiable         (:=)
 * - build set default  <variable> "<value>";  # set the default value of a makefile variable (?=)
 * - build append       <variable> "<value>";  # append to a makefile variable                (+=)
 **********************************************************************************************************************/
int build_parse (parser_t * p) {
	if (options == NULL) init_options();

	lex_item_t item = parser_skip(p, item_whitespace, 0);
	parse_fn fn = (parse_fn) hash_get(options, item.value);
	if (fn != NULL)  return fn(p);

	return errorf(p, item, "Expecting one of \n"
			"\t'depends', 'set', 'set default' or 'append', but got %s",
			lex_item_to_string(item)
			);
}

static int parse_depends(parser_t * p) {
	lex_item_t filename = parser_skip(p, item_whitespace, 0);
	if (filename.type != item_quoted_string) {
		return errorf(p, filename, "Expecting a filename but got '%s'", filename.value);
	}

	lex_item_t semicolon = parser_skip(p, item_whitespace, 0);
	if (semicolon.type != item_symbol && semicolon.value[0] != ';') {
		return errorf(p, semicolon, "Expecting ';' but got '%s'", lex_item_to_string(semicolon));
	}

	char * error = NULL;
	package_import_t * imp = package_import_add_c_file(p->pkg, string_parse(filename.value), &error);
	if (imp == NULL) {
		parser_errorf(p, filename, "", "Error adding dependency: %s", error);
		return -1;
	}
	return 1;
}

static int parse_set(parser_t * p) {
	bool is_default = false;
	lex_item_t name;
	do {
		name = parser_skip(p, item_whitespace, 0);

		if (name.type != item_id) {
			return errorf(p, name, "Expecting a variable name, but got %s", name.value);
		}

		if (strcmp(name.value, "default") == 0) {
			name.type = 0;
			is_default = true;
		}
	} while (name.type == 0);

	lex_item_t value = parser_skip(p, item_whitespace, 0);

	if (value.type != item_quoted_string) {
		return errorf(p, value, "Experting a quoted string, but got '%s'", value.value);
	}

	lex_item_t semicolon = parser_skip(p, item_whitespace, 0);
	if (semicolon.type != item_symbol && semicolon.value[0] != ';') {
		return errorf(p, semicolon, "Expecting ';' but got '%s'", lex_item_to_string(semicolon));
	}

	package_var_t v = {0};
	v.name      = name.value;
	v.value     = string_parse(value.value);
	v.operation = is_default ? build_var_set_default : build_var_set;

	p->pkg->variables = realloc(p->pkg->variables, sizeof(package_var_t) * (p->pkg->n_variables + 1));
	p->pkg->variables[p->pkg->n_variables] = v;
	p->pkg->n_variables++;

	return 1;
}

static int parse_append(parser_t * p) {
	lex_item_t name = parser_skip(p, item_whitespace, 0);

	if (name.type != item_id) {
		return errorf(p, name, "Expecting a variable name, but got %s", name.value);
	}

	lex_item_t value = parser_skip(p, item_whitespace, 0);

	if (value.type != item_quoted_string) {
		return errorf(p, value, "Experting a quoted string, but got '%s'", value.value);
	}

	lex_item_t semicolon = parser_skip(p, item_whitespace, 0);

	if (semicolon.type != item_symbol && semicolon.value[0] != ';') {
		return errorf(p, semicolon, "Expecting ';' but got '%s'", lex_item_to_string(semicolon));
	}

	package_var_t v = {0};
	v.name      = name.value;
	v.value     = string_parse(value.value);
	v.operation = build_var_append;

	p->pkg->variables = realloc(p->pkg->variables, sizeof(package_var_t) * (p->pkg->n_variables + 1));
	p->pkg->variables[p->pkg->n_variables] = v;
	p->pkg->n_variables++;
	return 1;
}