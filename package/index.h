#ifndef _package_index_
#define _package_index_

char * index_generated_name(const char * path);

#include "package.h"

package_t * index_new(const char * relative_path, char ** error, bool force, bool silent);

#include "../deps/stream/stream.h"

package_t * index_parse(
		stream_t   * input,
		stream_t   * out,
		const char * rel,
		char       * key,
		char       * generated,
		char      ** error,
		bool         force,
		bool         silent
);

#endif
