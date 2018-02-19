

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include "../deps/stream/stream.h"

static int _type;

int atomic_stream_type() {
	if (_type == 0) {
		_type = stream_register("atomic");
	}

	return _type;
}

typedef struct {
	int    fd;
	char * temp;
	char * dest;
} context_t;

static ssize_t atomic_write(void * _ctx, const void * buf, size_t nbyte, stream_error_t * error) {
	context_t * ctx = (context_t*) _ctx;
	int e = write(ctx->fd, buf, nbyte);
	if (e < 0 && error != NULL) {
		error->code    = errno;
		error->message = strerror(error->code);
	}
	return e;
}

static ssize_t atomic_close(void * _ctx, stream_error_t * error) {
	context_t * ctx = (context_t*) _ctx;

	int e = close(ctx->fd);
	if (e < 0 && error != NULL) {
		error->code    = errno;
		error->message = strerror(error->code);
		unlink(ctx->temp);
		free(ctx->temp);
		free(ctx->dest);
		free(ctx);
		return e;
	}

	e = rename(ctx->temp, ctx->dest);
	if (e < 0 && error != NULL) {
		error->code    = errno;
		error->message = strerror(error->code);
	}
	free(ctx->temp);
	free(ctx->dest);
	free(ctx);
	return e;
}

static char * get_temp(char * base) {
	int salt = rand();
	char * temp = NULL;
	char * ext  = strrchr(base, '.');

	asprintf(&temp, "%.*s-%x%s", (int)(ext - base), base, salt, ext);
	return temp;
}

stream_t * atomic_stream_open(const char * _dest) {
	int fd = 0;
	char * dest = strdup(_dest);
	char * temp = NULL;
	do {
		temp = get_temp(dest);
		fd   = open(temp, O_WRONLY | O_CREAT | O_EXCL, 0666);
	} while(fd == -1 && errno == EEXIST);

	if ( fd < 0 ) {
		free(dest);
		free(temp);
		return stream_error(NULL, errno, strerror(errno));
	}

	context_t * ctx = malloc(sizeof(context_t));
	ctx->fd   = fd;
	ctx->temp = temp;
	ctx->dest = strdup(dest);

	stream_t * s = malloc(sizeof(stream_t));

	s->ctx   = ctx;
	s->read  = NULL;
	s->write = atomic_write;
	s->pipe  = NULL;
	s->close = atomic_close;
	s->type  = atomic_stream_type();

	s->error.code    = 0;
	s->error.message = NULL;

	return s;
}

ssize_t atomic_stream_abort(stream_t * s) {
	if (s->type != atomic_stream_type()) return stream_close(s);

	context_t * ctx = (context_t*) s->ctx;

	close(ctx->fd);
	int e = unlink(ctx->temp);
	if (e < 0) {
		s->error.code    = errno;
		s->error.message = strerror(s->error.code);
	}
	free(ctx->temp);
	free(ctx->dest);
	free(ctx);
	return e;
}
