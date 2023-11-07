#include <stdlib.h>

#include "x.h"

void *__alloc(void *ptr, size_t size) {
  void *__ptr = NULL;
  if (size == 0)
    free(ptr);
  else if (ptr)
    __ptr = realloc(ptr, size);
  else
    __ptr = malloc(size);
  return __ptr;
}
