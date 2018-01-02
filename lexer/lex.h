#ifndef _package_lex_
#define _package_lex_

#include <stdlib.h>
#include <stdbool.h>

struct lex_lexer_s;

typedef void * (*lex_state_fn)(struct lex_lexer_s * lex);

#include "../deps/stream/stream.h"
#include "buffer.h"

typedef struct lex_lexer_s{
  stream_t * in;
  char     * input;
  char     * filename;
  size_t     length;
  size_t     start;
  size_t     pos;
  size_t     width;
  size_t     line;
  size_t     line_pos;
  lex_buffer_t * items;
  lex_state_fn   state;
} lex_t;

lex_t * lex_new(lex_state_fn start, stream_t * in, const char * filename);
lex_state_fn lex_errorf(lex_t * lex, const char * fmt, ...);
char lex_next(lex_t * lex);
void lex_backup(lex_t * lex);
char lex_peek(lex_t * lex);
bool lex_accept(lex_t * lex, const char * matching);
void lex_accept_run(lex_t * lex, const char * matching);
void lex_ignore(lex_t * lex);

#include "item.h"

void lex_emit(lex_t * lex, enum lex_item_type it);
lex_item_t lex_next_item(lex_t * lex);
void lex_free(lex_t * lex);

#endif
