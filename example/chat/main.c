#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

#include "../../ev.h"
#include "../../list.h"
#include "../../net.h"
#include "../../x.h"

#define BUF_MAX 1024

struct conn {
  struct list_head node;
  struct sockaddr_storage sa;
  struct ev ev;
  struct server *S;
  char buf[BUF_MAX];
};

struct server {
  struct ev ev;
  struct list_head conns;
};

int on_read(struct loop *L, struct ev *ev) {
  int n;
  struct conn *C, *p;
  struct list_head *el, *head;
  C = container_of(ev, struct conn, ev);
  if ((n = sock_read(C->ev.fd, C->buf, BUF_MAX)) < 0)
    return n;
  C->buf[n] = '\0';
  head = &C->S->conns;
  list_foreach(el, head) {
    p = container_of(el, struct conn, node);
    n = sock_write(p->ev.fd, C->buf, n);
    if (n < 0) {
      loop_del(L, &p->ev);
      list_del(el);
      close(p->ev.fd);
      xalloc(p, 0);
      printf("close %p\n", p);
    }
  }
  return 0;
}

int on_accept(struct loop *L, struct ev *ev) {
  struct conn *C;
  struct server *S = container_of(ev, struct server, ev);
  if ((C = xalloc(NULL, sizeof(*C))) == NULL)
    goto err;
  if ((C->ev.fd = tcp_accept(S->ev.fd, &C->sa)) < 0)
    goto err;
  C->ev.events = EV_READ;
  C->ev.callback = on_read;
  C->S = S;
  list_add(&C->node, &S->conns);
  loop_add(L, &C->ev);
  printf("accept %p\n", C);
  return 0;
err:
  if (C)
    xalloc(C, 0);
  return -1;
}

struct server *server_open(const char *host, unsigned short port) {
  struct server *S;
  struct ev *ev;
  if ((S = xalloc(NULL, sizeof(*S))) == NULL)
    goto err;
  ev = &S->ev;
  if ((ev->fd = tcp_listen(host, port)) < 0)
    goto err;
  ev->events = EV_READ;
  ev->callback = on_accept;
  list_head_init(&S->conns);
  return S;
err:
  if (S)
    xalloc(S, 0);
  return NULL;
}

void server_close(struct server *S) {
  struct conn *C;
  struct list_head *el, *head;
  list_foreach(el, head) {
    C = container_of(el, struct conn, node);
    close(C->ev.fd);
  }
  close(S->ev.fd);
}

int main(int argc, char **argv) {
  struct loop *L;
  struct server *S;
  int polled;

  L = loop_alloc(32);
  assert(L);

  S = server_open("127.0.0.1", 5555);
  assert(S);

  loop_add(L, &S->ev);

  polled = loop_wait(L);
  assert(polled > 0);

  server_close(S);
  loop_free(L);
  return 0;
}
