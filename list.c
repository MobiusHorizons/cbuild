#include <string.h>
#include <stdlib.h>

typedef struct {
  unsigned length;
  size_t allocated_size;
  void ** items; 
} List;

int list_push(List l, void* item){
  // check if we need to allocate memory
  if (l.length >= l.allocated_size){
    size_t next_size = (l.allocated_size > 0) ? l.allocated_size * 2 : sizeof(void*);
    l.items = (void **) realloc(l.items, next_size);
    if (l.items == NULL){
      return -1; // error
    }

    l.allocated_size = next_size;
  }

  // push the item;
  l.items[l.length++] = item;
  return l.length;
}
