#include "../../stream/stream.h"
#include "item.h"
#include "buffer.h"


#include <stdlib.h>
#include <stdbool.h>


#include <string.h>
#include <stdarg.h>
#include <stdio.h>

struct lex_lexer_s;
typedef void * (*lex_state_fn)(struct lex_lexer_s * lex);

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

static char * substring(const char * input, size_t start, size_t end);
static void count_newlines(lex_t * lex);


lex_t * lex_new(lex_state_fn start, stream_t * in, const char * filename) {
  lex_t * lex = calloc(1, sizeof(lex_t));

  lex->in       = in;
  lex->filename = strdup(filename);
  lex->input    = NULL;
  lex->length   = 0;
  lex->items    = lex_buffer_new(2);
  lex->state    = start;
  lex->line     = 0;

  return lex;
}

lex_state_fn lex_errorf(lex_t * lex, const char * fmt, ...) {
  va_list args;
  va_start(args, fmt);

  char * message = NULL;
  vasprintf(&message, fmt, args);
  lex_item_t error = lex_item_new(message, item_error, lex->line, lex->line_pos, lex->pos);
  lex->items = lex_buffer_push(lex->items, error);

  va_end(args);
  return NULL;
}

char lex_next(lex_t * lex) {
  if (lex->pos + 1 > lex->length) {
    if (lex->in->error.code != 0) {
      lex_errorf(lex, "Error reading input: %s", lex->in->error.message);
      return 0;
    }

    size_t next_length = lex->length + 4096 + 1;
    lex->input = realloc(lex->input, next_length);
    ssize_t len = stream_read(lex->in, lex->input + lex->length, 4096);

    if (len < 0) {
      lex_errorf(lex, "Error reading input: %s", lex->in->error.message);
      return 0;
    }

    if (len == 0) return 0;

    lex->length += len;
    lex->input[lex->length] = 0;
  }

  lex->width = 1;
  return lex->input[lex->pos++];
}

void lex_backup(lex_t * lex) {
  lex->pos -= lex->width;
}

char lex_peek(lex_t * lex) {
  char c = lex_next(lex);
  lex_backup(lex);
  return c;
}

bool lex_accept(lex_t * lex, const char * matching) {
  char t = lex_next(lex);
  const char *  c = matching;
  while (*c != '\0') {
    if (t == *c) return true;
    c++;
  }
  return false;
}

void lex_accept_run(lex_t * lex, const char * matching) {
  while (lex_accept(lex, matching));
  lex_backup(lex);
}

void lex_ignore(lex_t * lex) {
  lex->start  = lex->pos;
}

void lex_emit(lex_t * lex, enum lex_item_type it) {
  count_newlines(lex);
  lex_item_t i = lex_item_new (
      substring(lex->input, lex->start, lex->pos),
      it, lex->line, lex->line_pos, lex->start
  );

  lex->items = lex_buffer_push(lex->items, i);
  lex->start = lex->pos;
}

lex_item_t lex_next_item(lex_t * lex) {
  while(lex->items->length == 0 && lex->state != NULL) {
    lex->state = (lex_state_fn) lex->state(lex);
  }
  return lex_buffer_next(lex->items);
}

void lex_free(lex_t * lex) {
  stream_close(lex->in);

  free(lex->filename);
  free(lex->input);
  free(lex);
}

static char * substring(const char * input, size_t start, size_t end) {
  return strndup(input + start, end - start);
}

static void count_newlines(lex_t * lex) {
  size_t i;
  for (i = lex->start; i < lex->pos; i++) {
    if (lex->input[i] == '\n') {
      lex->line++;
      lex->line_pos = i+1;
    }
  }
}
