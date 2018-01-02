
#include "item.h"
#include <string.h>

#include <stdlib.h>


typedef struct {
  lex_item_t  * items;
  size_t        capacity;
  size_t        length;
} lex_item_stack_t;

lex_item_stack_t * lex_item_stack_new(size_t count) {
  lex_item_stack_t * s = malloc(sizeof(lex_item_stack_t));

  s->items    = malloc(count * sizeof(lex_item_t));
  s->capacity = count;
  s->length   = 0;

  return s;
}

lex_item_stack_t * lex_item_stack_resize(lex_item_stack_t * s, size_t count) {
  if (s->capacity >= count) {
    return s;
  }

  s->items = realloc(s->items, count * sizeof(lex_item_t));

  s->capacity = count;
  return s;
}

lex_item_stack_t * lex_item_stack_push(lex_item_stack_t * s, lex_item_t item) {
  s = lex_item_stack_resize(s, s->length + 1);

  s->items[s->length] = item;
  s->length++;

  return s;
}

lex_item_t lex_item_stack_pop(lex_item_stack_t * s) {
  if (s->length == 0) {
    return lex_item_new("No more items", item_error, 0,0,0);
  }

  s->length--;
  return s->items[s->length];
}

void lex_item_stack_free(lex_item_stack_t * s) {
  free(s->items);
  free(s);
}