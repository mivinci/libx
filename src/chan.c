#include <stddef.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "x/ev.h"
#include "x/x.h"

struct buf {
  int head;
  int tail;
  int cap;
  char *ptr;
};

struct chan {
  struct ev ev;
  struct buf recvq;
  struct buf sendq;
  int (*recv)(struct chan *, const char *, size_t, void *, int);
  int (*close)(struct chan *);
  int type;
};

int on_recv(struct loop *L, struct ev *ev) {
  struct chan *ch;
  struct buf *buf;
  struct sockaddr_storage sa, *psa = 0;
  socklen_t sa_size = 0;
  int n, m, l;

  ch = container_of(ev, struct chan, ev);
  buf = &ch->recvq;
  l = buf->cap - buf->tail;
  if (l <= 0) {
    if (buf->head <= 0)
      goto cb;
    else {
      memcpy(buf->ptr, buf->ptr + buf->head, buf->tail - buf->head);
      buf->tail -= buf->head;
      l = buf->cap - buf->tail;
    }
  }

  switch (ch->type) {
  case SOCK_DGRAM:
    n = recvfrom(ev->fd, buf->ptr + buf->tail, l, 0,  //
                 (struct sockaddr *)&sa, &sa_size);
    psa = &sa;
    break;
  default:
    n = read(ev->fd, buf->ptr + buf->tail, l);
  }
  if (n == 0)
    return ch->close(ch);
  else if (n < 0)
    return n;
  buf->tail = buf->head + n;
cb:
  m = ch->recv(ch, buf->ptr + buf->head, buf->tail - buf->head, psa, sa_size);
  buf->head += m;
  return 0;
}

int on_send(struct loop *L, struct ev *ev) {}
