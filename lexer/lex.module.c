#define _GNU_SOURCE
import stream from "../deps/stream/stream.module.c";
import item   from "./item.module.c";
import buffer from "./buffer.module.c";

export {
#include <stdlib.h>
#include <stdbool.h>
}

#include <string.h>
#include <stdarg.h>
#include <stdio.h>

export struct lexer_s;
export typedef void * (*state_fn)(struct lexer_s * lex);

export typedef struct lexer_s{
  stream.t * in;
  char     * input;
  char     * filename;
  size_t     length;
  size_t     start;
  size_t     pos;
  size_t     width;
  size_t     line;
  size_t     line_pos;
  buffer.t * items;
  state_fn   state;
} lexer_t as t;

static char * substring(const char * input, size_t start, size_t end);
static void count_newlines(lexer_t * lex);


export lexer_t * new(state_fn start, stream.t * in, const char * filename) {
  lexer_t * lex = calloc(1, sizeof(lexer_t));

  lex->in       = in;
  lex->filename = strdup(filename);
  lex->input    = NULL;
  lex->length   = 0;
  lex->items    = buffer.new(2);
  lex->state    = start;
  lex->line     = 0;

  return lex;
}

export state_fn errorf(lexer_t * lex, const char * fmt, ...) {
  va_list args;
  va_start(args, fmt);

  char * message = NULL;
  vasprintf(&message, fmt, args);
  item.t error = item.new(message, item_error, lex->line, lex->line_pos, lex->pos);
  lex->items = buffer.push(lex->items, error);

  va_end(args);
  return NULL;
}

export char next(lexer_t * lex) {
  if (lex->pos + 1 > lex->length) {
    if (lex->in->error.code != 0) {
      errorf(lex, "Error reading input: %s", lex->in->error.message);
      return 0;
    }

    size_t next_length = lex->length + 4096 + 1;
    lex->input = realloc(lex->input, next_length);
    ssize_t len = stream.read(lex->in, lex->input + lex->length, 4096);

    if (len < 0) {
      errorf(lex, "Error reading input: %s", lex->in->error.message);
      return 0;
    }

    if (len == 0) return 0;

    lex->length += len;
    lex->input[lex->length] = 0;
  }

  lex->width = 1;
  return lex->input[lex->pos++];
}

export void backup(lexer_t * lex) {
  lex->pos -= lex->width;
}

export char peek(lexer_t * lex) {
  char c = next(lex);
  backup(lex);
  return c;
}

export bool accept(lexer_t * lex, const char * matching) {
  char t = next(lex);
  const char *  c = matching;
  while (*c != '\0') {
    if (t == *c) return true;
    c++;
  }
  return false;
}

export void accept_run(lexer_t * lex, const char * matching) {
  while (accept(lex, matching));
  backup(lex);
}

export void ignore(lexer_t * lex) {
  lex->start  = lex->pos;
}

export void emit(lexer_t * lex, enum item.type it) {
  count_newlines(lex);
  item.t i = item.new (
      substring(lex->input, lex->start, lex->pos),
      it, lex->line, lex->line_pos, lex->start
  );

  lex->items = buffer.push(lex->items, i);
  lex->start = lex->pos;
}

export item.t next_item(lexer_t * lex) {
  while(lex->items->length == 0 && lex->state != NULL) {
    lex->state = (state_fn) lex->state(lex);
  }
  return buffer.next(lex->items);
}

export void free(lexer_t * lex) {
  stream.close(lex->in);

  global.free(lex->filename);
  global.free(lex->input);
  global.free(lex);
}

static char * substring(const char * input, size_t start, size_t end) {
  return strndup(input + start, end - start);
}

static void count_newlines(lexer_t * lex) {
  size_t i;
  for (i = lex->start; i < lex->pos; i++) {
    if (lex->input[i] == '\n') {
      lex->line++;
      lex->line_pos = i+1;
    }
  }
}

