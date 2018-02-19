#ifndef _package_package_
#define _package_package_

#include "../deps/hash/hash.h"
#include <stdlib.h>
#include <stdbool.h>

enum package_var_type {
	build_var_set = 0,
	build_var_set_default,
	build_var_append,
};

typedef struct {
	char * name;
	char * value;
	enum package_var_type operation;
} package_var_t;

#include "../deps/stream/stream.h"

typedef struct {
	hash_t   * deps;
	hash_t   * exports;
	hash_t   * symbols;
	void    ** ordered;
	size_t     n_exports;
	package_var_t    * variables;
	size_t     n_variables;
	char     * name;
	char     * source_abs;
	char     * generated;
	char     * header;
	size_t     errors;
	bool       exported;
	bool       c_file;
	bool       force;
	bool       silent;
	stream_t * out;
} package_t;

extern hash_t * package_path_cache;
extern hash_t * package_id_cache;
extern package_t * (*package_new)(const char * relative_path, char ** error, bool force, bool silent);
void package_emit(package_t * pkg, char * value);
package_t * package_c_file(char * abs_path, char ** error);
void package_free(package_t * pkg);

#endif
