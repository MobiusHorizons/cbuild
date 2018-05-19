#ifndef _package_package_export_
#define _package_package_export_

enum package_export_type {
	type_block = 0,
	type_type,
	type_function,
	type_enum,
	type_union,
	type_struct,
	type_header,
};

typedef struct {
	char      * local_name;
	char      * export_name;
	char      * declaration;
	char      * symbol;
	enum package_export_type   type;
} package_export_t;

void package_export_free(package_export_t * exp);

#include "package.h"

char * package_export_add(char * local, char * alias, char * symbol, char * type, char * declaration, package_t * parent);
void package_export_write_headers(package_t * pkg);
void package_export_export_headers(package_t * pkg, package_t * dep);

#endif
