#include <stdlib.h>

#include "x/mm.h"

void *__alloc(void *ptr, size_t size) {
  if (size != 0)
    return realloc(ptr, size);
  else {
    free(ptr);
    return NULL;
  }
}
