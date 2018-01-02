#ifndef _string_stream_
#define _string_stream_

#include <fcntl.h>

#include "../../stream/stream.h"

int string_stream_type();
stream_t * string_stream_new_reader(const char * input);
stream_t * string_stream_new_writer();
char * string_stream_get_buffer(stream_t * s);

#endif
