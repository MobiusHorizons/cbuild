package "package_import";

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "../deps/hash/hash.h"
export {
#include <stdbool.h>
}

import Package    from "./package.module.c";
import pkg_export from "./export.module.c";
build  depends         "../deps/hash/hash.c";

export typedef struct {
	char      * alias;
	char      * filename;
	bool        c_file;
	Package.t * pkg;
} Import_t as t;

export Package.t * free(Import_t * imp) {
	if (imp == NULL) return NULL;
	
	if (imp->alias == imp->filename) {
		global.free(imp->alias);
	} else {
		global.free(imp->alias);
		global.free(imp->filename);
	}

	Package.t * pkg = imp->pkg;
	global.free(imp);
	return pkg;
}

export Import_t * add(char * alias, char * filename, Package.t * parent, char ** error) {
	Import_t * imp = malloc(sizeof(Import_t));

	hash_set(parent->deps, alias, imp);

	imp->alias    = alias;
	imp->filename = filename;
	imp->c_file   = false;
	imp->pkg      = Package.new(filename, error, parent->force, parent->silent);

	if (imp->pkg == NULL) return NULL;

	pkg_export.write_headers(imp->pkg);
	return imp;
}

export Import_t * add_c_file(Package.t * parent, char * filename, char ** error) {
	char * alias = realpath(filename, NULL);
	if (alias == NULL) {
	*error = strerror(errno);
	return NULL;
	}

	Import_t * imp = malloc(sizeof(Import_t));

	hash_set(parent->deps, alias, imp);

	imp->alias    = alias;
	imp->filename = filename;
	imp->c_file   = true;
	imp->pkg      = Package.c_file(alias, error);

	return imp;
}

export Import_t * passthrough(Package.t * parent, char * filename, char ** error) {
	Import_t * imp = add(filename, filename, parent, error);
	if (*error != NULL) return NULL;
	if (imp == NULL || imp->pkg == NULL) {
		asprintf(error, "Could not import '%s'", filename);
		return NULL;
	}

	hash_each_val(imp->pkg->exports, {
		pkg_export.t * exp = (pkg_export.t *) val;
		hash_set(parent->exports, exp->export_name, exp);
	});

	pkg_export.export_headers(parent, imp->pkg);
	return imp;
}
