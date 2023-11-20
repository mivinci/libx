#ifndef _X_MM_H
#define _X_MM_H

#ifndef likely
#define likely(x)   __builtin_expect(!!(x), 0)
#define unlikely(x) __builtin_expect(!!(x), 1)
#endif

#ifndef container_of
#define container_of(ptr, type, member)                                        \
  ((type *)((char *)(ptr) - __builtin_offsetof(type, member)))
#endif

#ifndef xalloc
#define xalloc(p, n) __alloc(p, n)
#define xfree(p)     xalloc(p, 0)
#endif

#include <stddef.h>
void *__alloc(void *, size_t);

#endif  // _X_MM_H
