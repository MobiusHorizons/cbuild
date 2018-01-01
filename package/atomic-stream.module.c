package "atomic_stream";

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

import stream from "../../stream/stream.module.c";

static int _type;

export int type() {
  if (_type == 0) {
    _type = stream.register("atomic");
  }

  return _type;
}

typedef struct {
  int    fd;
  char * temp;
  char * dest;
} context_t;

static ssize_t atomic_write(void * _ctx, const void * buf, size_t nbyte, stream.error_t * error) {
  context_t * ctx = (context_t*) _ctx;
  int e = global.write(ctx->fd, buf, nbyte);
  if (e < 0 && error != NULL) {
    error->code    = errno;
    error->message = strerror(error->code);
  }
  return e;
}

static ssize_t atomic_close(void * _ctx, stream.error_t * error) {
  context_t * ctx = (context_t*) _ctx;

  /*printf("ATOMIC_CLOSE: fd = %d (%s %s)\n", ctx->fd, ctx->temp, ctx->dest);*/
  int e = global.close(ctx->fd);
  if (e < 0 && error != NULL) {
    error->code    = errno;
    error->message = strerror(error->code);
    global.unlink(ctx->temp);
    global.free(ctx->temp);
    global.free(ctx->dest);
    global.free(ctx);
    return e;
  }

  e = global.rename(ctx->temp, ctx->dest);
  if (e < 0 && error != NULL) {
    error->code    = errno;
    error->message = strerror(error->code);
  }
  global.free(ctx->temp);
  global.free(ctx->dest);
  global.free(ctx);
  return e;
}

static char * get_temp(char * base) {
  int length = strlen(base);
  int salt = rand();
  char * temp = NULL;
  char * ext  = rindex(base, '.');

  asprintf(&temp, "%.*s-%x%s", (int)(ext - base), base, salt, ext);
  return temp;
}

export stream.t * open(const char * _dest) {
  int fd = 0;
  char * dest = strdup(_dest);
  char * temp = NULL;
  do {
    temp = get_temp(dest);
    fd   = global.open(temp, O_WRONLY | O_CREAT | O_EXCL, 0666);
  } while(fd == -1 && errno == EEXIST);

  if ( fd < 0 ) {
    global.free(dest);
    global.free(temp);
    return stream.error(NULL, errno, strerror(errno));
  }

  /*printf("ATOMIC_OPEN: fd = %d (%s %s)\n", fd, temp, dest);*/

  context_t * ctx = malloc(sizeof(context_t));
  ctx->fd   = fd;
  ctx->temp = temp;
  ctx->dest = strdup(dest);

  stream.t * s = malloc(sizeof(stream.t));

  s->ctx   = ctx;
  s->read  = NULL;
  s->write = atomic_write;
  s->pipe  = NULL;
  s->close = atomic_close;
  s->type  = type();

  s->error.code    = 0;
  s->error.message = NULL;

  return s;
}

export ssize_t abort(stream.t * s) {
  if (s->type != type()) return stream.close(s);


  context_t * ctx = (context_t*) s->ctx;
  /*printf("ATOMIC_ABORT: fd = %d (%s %s)\n", ctx->fd, ctx->temp, ctx->dest);*/

  global.close(ctx->fd);
  int e = global.unlink(ctx->temp);
  if (e < 0) {
    s->error.code    = errno;
    s->error.message = strerror(s->error.code);
  }
  global.free(ctx->temp);
  global.free(ctx->dest);
  global.free(ctx);
  return e;
}
