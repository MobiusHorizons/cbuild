package "lex_item_stack";
import lex_item from "./item.module.c";
#include <string.h>
export {
#include <stdlib.h>
}

export typedef struct {
	lex_item.t  * items;
	size_t        capacity;
	size_t        length;
} item_stack_t as t;

export item_stack_t * new(size_t count) {
	item_stack_t * s = malloc(sizeof(item_stack_t));

	s->items    = malloc(count * sizeof(lex_item.t));
	s->capacity = count;
	s->length   = 0;

	return s;
}

export item_stack_t * resize(item_stack_t * s, size_t count) {
	if (s->capacity >= count) {
		return s;
	}

	s->items = realloc(s->items, count * sizeof(lex_item.t));

	s->capacity = count;
	return s;
}

export item_stack_t * push(item_stack_t * s, lex_item.t item) {
	s = resize(s, s->length + 1);

	s->items[s->length] = item;
	s->length++;

	return s;
}

export lex_item.t pop(item_stack_t * s) {
	if (s->length == 0) {
		return lex_item.new("No more items", item_error, 0,0,0);
	}

	s->length--;
	return s->items[s->length];
}

export void free(item_stack_t * s) {
	int i;
	for(i = 0; i < s->length; i++) {
		lex_item.free(s->items[i]);
	}
	s->length = 0;

	global.free(s->items);
	s->items = NULL;

	global.free(s);
}
