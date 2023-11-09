#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include "x/ev.h"

#define RBUF_MAX 1024

int echo(struct loop *L, struct ev *ev) {
  if (ev->revents & EV_TIMER) {
    printf("timeout %d.%d\n", ev->exp.tv_sec, ev->exp.tv_usec);
    loop_del(L, ev);
    return 0;
  }
  char buf[RBUF_MAX];
  int n;
  if ((n = read(ev->fd, buf, RBUF_MAX)) < n)
    return n;
  return write(STDOUT_FILENO, buf, n);
}

int main(void) {
  struct loop *L;

  L = loop_alloc(1);
  assert(L);

  struct ev ev = {
      .events = EV_READ | EV_TIMER,
      .fd = STDIN_FILENO,
      .callback = echo,
      .ms = 4 * 1000,
  };
  loop_add(L, &ev);

  loop_wait(L);
  loop_free(L);
  return 0;
}
