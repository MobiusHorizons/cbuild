build depends "../deps/hash/hash.c";
#include "../deps/hash/hash.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <libgen.h>

import stream  from "../deps/stream/stream.module.c";
import file    from "../deps/stream/file.module.c";
import grammer from "../parser/grammer.module.c";
import parser  from "../parser/parser.module.c";
import utils   from "../utils/utils.module.c";
import Package from "./package.module.c";
import Import  from "./import.module.c";
import Export  from "./export.module.c";
import atomic  from "./atomic-stream.module.c";

static void init_cache() {
	Package.path_cache = hash_new();
	Package.id_cache   = hash_new();
}

static char * abs_path(const char * rel_path) {
	return realpath(rel_path, NULL);
}

static char * package_name(const char * rel_path) {
	char * buffer   = strdup(rel_path);
	char * filename = basename(buffer);

	filename[strlen(filename) - 2] = 0; // trim the .c extension

	char * c;
	for (c = filename; *c != 0; c++){
		switch(*c) {
			case '.':
			case '-':
			case ' ':
				*c = '_';
				break;
			default:
				break;
		}
	}

	char * name = strdup(filename);
	free(buffer);
	return name;
}

static bool assert_name(const char * relative_path, char ** error) {
	ssize_t len      = strlen(relative_path);
	size_t  suffix_l = strlen(".module.c");

	if (len < suffix_l || strcmp(".module.c", relative_path + len - suffix_l) != 0) {
		if (error) asprintf(error, "Unsupported input filename '%s', Expecting '<file>.module.c'", relative_path);
		return true;
	}
	return false;
}

export char * generated_name(const char * path) {
	ssize_t len      = strlen(path);
	size_t  suffix_l = strlen(".module.c");
	char * buffer    = malloc(len - suffix_l + strlen(".c") + 1);
	strncpy(buffer, path, len - suffix_l);
	strcpy(buffer + (len - suffix_l), ".c");
	return buffer;
}

export Package.t * new(const char * relative_path, char ** error, bool force, bool silent);

export Package.t * parse(
		stream.t   * input,
		stream.t   * out,
		const char * rel,
		char       * key,
		char       * generated,
		char      ** error,
		bool         force,
		bool         silent
) {
	if (Package.path_cache == NULL) init_cache();
	if (Package.new        == NULL) Package.new = new;

	Package.t * p = calloc(1, sizeof(Package.t));
	p->deps       = hash_new();
	p->exports    = hash_new();
	p->ordered    = NULL;
	p->n_exports  = 0;
	p->symbols    = hash_new();
	p->source_abs = key;
	p->generated  = generated;
	p->out        = out;
	p->name       = package_name(p->generated);
	p->force      = force;
	p->silent     = silent;


	hash_set(Package.path_cache, key, p);

	p->errors = grammer.parse(input, rel, p, error);
	return p;
}

Package.t * new(const char * relative_path, char ** error, bool force, bool silent) {
	if (Package.path_cache == NULL) init_cache();
	if (Package.new        == NULL) Package.new = new;

	if (assert_name(relative_path, error)) return NULL;
	char * key = abs_path(relative_path);

	if (key == NULL) {
		*error = strerror(errno);
		return NULL;
	}

	Package.t * cached = hash_get(Package.path_cache, key);
	if (cached != NULL) {
		free(key);
		return cached;
	}

	stream.t * input = file.open(relative_path, O_RDONLY);
	if (input->error.code != 0) {
		*error = strdup(input->error.message);
	free(key);
		return NULL;
	}

	char * generated = generated_name(key);
	stream.t * out = NULL;
	if (force || (!silent && utils.newer(relative_path, generated))) {
		out = atomic.open(generated);
		if (out->error.code != 0) {
		free(key);
			if (error) *error = strdup(out->error.message);
			return NULL;
		}
	}

	Package.t * p = parse(input, out, relative_path, key, generated, error, force, silent);

	if (p == NULL || *error != NULL) {
	free(key);
		if (out) atomic.abort(out);
		return NULL;
	}

	if (out) stream.close(out);
	return p;
}
