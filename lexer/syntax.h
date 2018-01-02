#ifndef _package_lex_syntax_
#define _package_lex_syntax_

#include "lex.h"
#include "../deps/stream/stream.h"

lex_t * lex_syntax_new(stream_t * input, const char * filename, char ** error);

#endif
