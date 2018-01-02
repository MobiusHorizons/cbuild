

#include "item.h"
#include <string.h>

#include <stdlib.h>


typedef struct {
  lex_item_t  * items;
  size_t        capacity;
  size_t        length;
  size_t        cursor;
} lex_buffer_t;

lex_buffer_t * lex_buffer_new(size_t count) {
  lex_buffer_t * b = malloc(sizeof(lex_buffer_t));

  b->items    = malloc(count * sizeof(lex_item_t));
  b->capacity = count;
  b->length   = 0;
  b->cursor   = 0;

  return b;
}

lex_buffer_t * lex_buffer_resize(lex_buffer_t * b, size_t count) {
  if (b->capacity >= count) {
    return b;
  }

  b->items = realloc(b->items, count * sizeof(lex_item_t));

  if (b->cursor > 0 && b->length + b->cursor > b->capacity) {
    int i;
    for (i = 0; i < (count - b->capacity); i++) {
      b->items[i + b->capacity] = b->items[i];
    }
  }

  b->capacity = count;
  return b;
}

lex_buffer_t * lex_buffer_push(lex_buffer_t * b, lex_item_t item) {
  b = lex_buffer_resize(b, b->length + 1);

  size_t last = (b->length + b->cursor) % b->capacity;

  b->items[last] = item;
  b->length++;

  return b;
}

lex_item_t lex_buffer_next(lex_buffer_t * b) {
  if (b->length == 0) {
    return lex_item_new("No more items", item_error, 0,0,0);
  }

  lex_item_t i = b->items[b->cursor];
  b->cursor = ((b->cursor + 1) < b->capacity) ? b->cursor + 1 : 0;
  b->length--;

  return i;
}

void lex_buffer_free(lex_buffer_t * b) {
  free(b->items);
  free(b);
}