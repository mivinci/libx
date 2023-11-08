#include <assert.h>
#include <stdio.h>

#include "x/ev.h"

int f(struct loop *L, struct ev *ev) {
  assert(ev->revents == EV_TIMER);
  printf("timeout %d.%d\n", ev->exp.tv_sec, ev->exp.tv_usec);
  loop_add(L, ev);
}

int main(void) {
  struct loop *L;

  L = loop_alloc(1);
  assert(L);

  struct ev ev;
  ev.events = EV_TIMER;
  ev.ms = 1000;
  ev.callback = f;
  loop_add(L, &ev);

  loop_wait(L);
  loop_free(L);
  return 0;
}
