

#include <stdlib.h>
#include <stdio.h>
#include "../deps/hash/hash.h"
#include <stdbool.h>

#include "package.h"
#include "../deps/stream/stream.h"
#include "atomic-stream.h"
#include "../utils/utils.h"


enum package_export_type {
	type_block = 0,
	type_type,
	type_function,
	type_enum,
	type_union,
	type_struct,
	type_header,
};

const char * type_names[] = {
	"block",
	"type",
	"function",
	"enum",
	"union",
	"struct",
	"header",
};

typedef struct {
	char      * local_name;
	char      * export_name;
	char      * declaration;
	char      * symbol;
	enum package_export_type   type;
	package_t * pkg;
} package_export_t;

static hash_t * types = NULL;
static void init_types() {
	types = hash_new();

	hash_set(types, "typedef",  (void*)type_type);
	hash_set(types, "function", (void*)type_function);
	hash_set(types, "enum",     (void*)type_enum);
	hash_set(types, "union",    (void*)type_union);
	hash_set(types, "struct",   (void*)type_struct);
	hash_set(types, "header",   (void*)type_header);
}

char * package_export_add(char * local, char * alias, char * symbol, char * type, char * declaration, package_t * parent) {
	if (types == NULL) init_types();

	package_export_t * exp = malloc(sizeof(package_export_t));

	exp->local_name  = local;
	exp->export_name = alias == NULL ? local : alias;
	exp->declaration = declaration;
	exp->type        = (enum package_export_type) hash_get(types, type);
	exp->symbol      = symbol;

	if (!hash_has(parent->exports, exp->export_name)) {
		parent->ordered = realloc(parent->ordered, sizeof(void*) * (parent->n_exports + 1));
		parent->ordered[parent->n_exports] = exp;
		parent->n_exports ++;
	}

	hash_set(parent->exports, exp->export_name, exp);
	hash_set(parent->symbols, exp->local_name,  exp);

	return exp->export_name;
}

static char * get_header_path(char * generated) {
	char * header = strdup(generated);
	size_t len = strlen(header);
	header[len - 1] = 'h';
	return header;
}

#define B(a) (a ? "true" : "false")

void package_export_write_headers(package_t * pkg) {
	if (pkg->header) return;

	pkg->header = get_header_path(pkg->generated);

	if (pkg->force == false && (pkg->silent || !utils_newer(pkg->source_abs, pkg->header))) return;

	stream_t * header = atomic_stream_open(pkg->header);
	stream_printf(header, "#ifndef _package_%s_\n" "#define _package_%s_\n\n", pkg->name, pkg->name);

	enum package_export_type last_type;
	bool had_newline   = true;

	int i;
	for (i = 0; i < pkg->n_exports; i++){
			package_export_t * exp = (package_export_t *) pkg->ordered[i];
			bool newline   = false;
			bool prefix    = false;
			bool multiline = strchr(exp->declaration, '\n') != NULL;

			if (had_newline == false && (last_type != exp->type || multiline)) prefix = true;
			if (multiline) newline = true;

			stream_printf(header, "%s%s\n%s", prefix ? "\n" : "", exp->declaration, newline ? "\n" : "");

			last_type     = exp->type;
			had_newline   = newline;
	}
	stream_printf(header, "%s#endif\n", had_newline ? "" : "\n");
	stream_close(header);
}

void package_export_export_headers(package_t * pkg, package_t * dep) {
	if (pkg == NULL || dep == NULL) return;
	package_export_write_headers(dep);

	char * rel  = utils_relative(pkg->source_abs, dep->header);
	char * decl = NULL;
	asprintf(&decl, "#include \"%s\"", rel);

	package_export_add(strdup(rel), NULL, strdup(""), "header", decl, pkg);
	free(rel);
}
