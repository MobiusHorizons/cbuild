#ifndef _package_string_stream_
#define _package_string_stream_

#include <fcntl.h>
int string_stream_type();

#include "../deps/stream/stream.h"

stream_t * string_stream_new_reader(const char * input);
stream_t * string_stream_new_writer();
char * string_stream_get_buffer(stream_t * s);

#endif
