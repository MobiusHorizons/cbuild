package "list";
#include <string.h>
#include <stdlib.h>

export typedef struct {
  unsigned length;
  size_t allocated_size;
  void ** items; 
} List;

export int push(List *l, void* item){
  // check if we need to allocate memory
  if (l->length >= (l->allocated_size / sizeof(void*))){
    size_t next_size = (l->allocated_size > 0) ? l->allocated_size * 2 : sizeof(void*);
    l->items = (void **) realloc(l->items, next_size);
    if (l->items == NULL){
      return -1; // error
    }

    l->allocated_size = next_size;
  }

  // push the item;
  l->items[l->length++] = item;
  return l->length;
};

export void * get(List * l, unsigned index){
    if (index < l->length){
        return l->items[index];
    }
    return NULL;
}
