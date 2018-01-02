#ifndef _package_lex_item_stack_
#define _package_lex_item_stack_

#include <stdlib.h>

#include "item.h"

typedef struct {
  lex_item_t  * items;
  size_t        capacity;
  size_t        length;
} lex_item_stack_t;

lex_item_stack_t * lex_item_stack_new(size_t count);
lex_item_stack_t * lex_item_stack_resize(lex_item_stack_t * s, size_t count);
lex_item_stack_t * lex_item_stack_push(lex_item_stack_t * s, lex_item_t item);
lex_item_t lex_item_stack_pop(lex_item_stack_t * s);
void lex_item_stack_free(lex_item_stack_t * s);

#endif
