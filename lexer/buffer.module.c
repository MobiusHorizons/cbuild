package "lex_buffer";

import lex_item from "./item.module.c";
#include <string.h>
export {
#include <stdlib.h>
}

export typedef struct {
	lex_item.t  * items;
	size_t        capacity;
	size_t        length;
	size_t        cursor;
} item_buffer_t as t;

export item_buffer_t * new(size_t count) {
	item_buffer_t * b = malloc(sizeof(item_buffer_t));

	b->items    = malloc(count * sizeof(lex_item.t));
	b->capacity = count;
	b->length   = 0;
	b->cursor   = 0;

	return b;
}

export item_buffer_t * resize(item_buffer_t * b, size_t count) {
	if (b->capacity >= count) {
		return b;
	}

	lex_item.t * items = malloc(count * sizeof(lex_item.t));


	int i;
	if (count > b->length) {
		for (i = 0; i < b->length; i++) {
			items[i] = b->items[(i + b->cursor) % b->capacity];
		}
	} else {
		// on shrink drop items from the end
		for (i = 0; i < count; i++) {
			items[i] = b->items[(i + b->cursor) % b->capacity];
		}
	}

	free(b->items);
	b->items = items;
	b->capacity = count;
	b->cursor = 0;

	return b;
}

export item_buffer_t * push(item_buffer_t * b, lex_item.t item) {
	b = resize(b, b->length + 1);
	size_t last = (b->length + b->cursor) % b->capacity;

	b->items[last] = item;
	b->length++;

	return b;
}

export lex_item.t next(item_buffer_t * b) {
	if (b->length == 0) {
		return lex_item.new("No more items", item_error, 0,0,0);
	}

	lex_item.t i = b->items[b->cursor];
	b->cursor = ((b->cursor + 1) < b->capacity) ? b->cursor + 1 : 0;
	b->length--;

	return i;
}

export void free(item_buffer_t * b) {
	while(b->length) {
		lex_item_free(next(b));
	}
	global.free(b->items);
	global.free(b);
}
