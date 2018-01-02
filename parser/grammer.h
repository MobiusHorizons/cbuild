#ifndef _package_grammer_
#define _package_grammer_

#include "../deps/stream/stream.h"
#include "../package/package.h"

int grammer_parse(stream_t * in, const char * filename, package_t * p, char ** error);

#endif
