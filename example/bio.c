#include <assert.h>
#include <stdio.h>

#include "x/ev.h"
#include "x/io.h"

ssize_t on_read(struct bio *B, const char *p, size_t n) {
  return printf("on_read (%ld) %s", n, p);
}

void on_close(struct bio *B) { 
  printf("on_close"); 
}

int main(void) {
  struct loop *L;
  L = loop_alloc(1);
  assert(L);

  struct bio *B;
  B = bio_alloc(L, STDIN_FILENO, 32, 32, on_read, on_close);
  assert(B);

  loop_wait(L);

  bio_free(B);
  loop_free(L);
  return 0;
}
