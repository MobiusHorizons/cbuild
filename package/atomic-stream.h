#ifndef _package_atomic_stream_
#define _package_atomic_stream_

int atomic_stream_type();

#include "../../stream/stream.h"

stream_t * atomic_stream_open(const char * _dest);
ssize_t atomic_stream_abort(stream_t * s);

#endif
