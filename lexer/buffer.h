#ifndef _package_lex_buffer_
#define _package_lex_buffer_

#include <stdlib.h>

#include "item.h"

typedef struct {
	lex_item_t  * items;
	size_t        capacity;
	size_t        length;
	size_t        cursor;
} lex_buffer_t;

lex_buffer_t * lex_buffer_new(size_t count);
lex_buffer_t * lex_buffer_resize(lex_buffer_t * b, size_t count);
lex_buffer_t * lex_buffer_push(lex_buffer_t * b, lex_item_t item);
lex_item_t lex_buffer_next(lex_buffer_t * b);
void lex_buffer_free(lex_buffer_t * b);

#endif
