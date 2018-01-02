#define _GNU_SOURCE
package "string_stream";
import stream from "../deps/stream/stream.module.c";

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
export {
#include <fcntl.h>
}

static int _type;

export int type() {
  if (_type == 0) {
    _type = stream.register("string");
  }

  return _type;
}

typedef struct {
  char * buf;
  size_t offset;
  size_t length;
} context_t;

static ssize_t string_read(void * _ctx, void * buf, size_t nbyte, stream.error_t * error) {
  context_t * ctx = (context_t *)_ctx;
  size_t len = nbyte > ctx->length ? ctx->length : nbyte;
  memcpy(buf, ctx->buf + ctx->offset, len);
  ctx->offset += len;
  ctx->length -= len;
  return len;
}

static ssize_t string_write(void * _ctx, const void * buf, size_t nbyte, stream.error_t * error) {
  context_t * ctx = (context_t *)_ctx;
  ctx->buf = realloc(ctx->buf, ctx->length + nbyte + 1);
  memcpy(ctx->buf + ctx->offset, buf, nbyte);
  ctx->offset += nbyte;
  ctx->length += nbyte;
  ctx->buf[ctx->offset] = 0;

  return nbyte;
}

static ssize_t file_close(void * _ctx, stream.error_t * error) {
  context_t * ctx = (context_t *)_ctx;
  global.free(ctx->buf);
  global.free(ctx);
  return 0;
}

export stream.t * new_reader(const char * input) {
  context_t *ctx = malloc(sizeof(context_t));
  ctx->buf    = strdup(input);
  ctx->length = strlen(input);
  ctx->offset = 0;

  stream.t * s = malloc(sizeof(stream.t));

  s->ctx   = ctx;
  s->read  = string_read;
  s->write = NULL;
  s->pipe  = NULL;
  s->close = file_close;
  s->type  = type();

  s->error.code    = 0;
  s->error.message = NULL;

  return s;
}

export stream.t * new_writer() {
  context_t *ctx = malloc(sizeof(context_t));
  ctx->buf    = NULL;
  ctx->length = 0;
  ctx->offset = 0;

  stream.t * s = malloc(sizeof(stream.t));

  s->ctx   = ctx;
  s->read  = NULL;
  s->write = string_write;
  s->pipe  = NULL;
  s->close = file_close;
  s->type  = type();

  s->error.code    = 0;
  s->error.message = NULL;

  return s;
}

export char * get_buffer(stream.t * s) {
  if (s->type != type()) return NULL;
  context_t * ctx = (context_t *)s->ctx;

  return ctx->buf;
}
