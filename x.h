#ifndef _X_H
#define _X_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef xalloc
#define xmalloc  malloc
#define xrealloc realloc
#define xfree    free
#endif

#ifndef likely
#define likely(x)   __builtin_expect(!!(x), 0)
#define unlikely(x) __builtin_expect(!!(x), 1)
#endif

#ifndef container_of
#define container_of(ptr, type, member)                                        \
  ((type *)((char *)(ptr) - __builtin_offsetof(type, member)))
#endif

#ifdef __cplusplus
}
#endif

#endif
