#include <assert.h>
#include <string.h>

#include "x/ev.h"
#include "x/io.h"
#include "x/x.h"

#define buf_len(p)   ((p)->tail - (p)->head)
#define buf_awail(p) ((p)->cap - (p)->tail)
#define buf_head(p)  ((p)->ptr + (p)->head)
#define buf_tail(p)  ((p)->ptr + (p)->tail)

struct buf {
  int head;
  int tail;
  int cap;
  char *ptr;
};

struct bio {
  struct ev ev;
  struct buf recvq;
  struct buf sendq;
  __bio_read read;
  __bio_close close;
};

static int on_read(struct loop *L, struct ev *ev) {
  struct bio *io;
  struct buf *buf;
  size_t n, m, l;

  io = container_of(ev, struct bio, ev);
  buf = &io->recvq;
  l = buf_awail(buf);
  if (l <= 1) {  // leave one byte for '\0'
    if (buf->head <= 0)
      goto cb;
    else {
      memcpy(buf->ptr, buf_head(buf), buf_len(buf));
      buf->tail -= buf->head;
      l = buf_awail(buf);
    }
  }

  n = read(ev->fd, buf_tail(buf), l);
  if (n < 0)
    return n;
  else if (n == 0) {
    if (io->close)
      io->close(io);
    return 0;
  }
  buf->tail = buf->head + n;
  *buf_tail(buf) = 0;
cb:
  n = buf_len(buf);
  m = io->read(io, buf_head(buf), n);
  if (m < 0)
    return m;
  buf->head += ((m < n) ? m : n);
  return 0;
}

// static int on_write(struct loop *L, struct ev *ev) {
//   struct bio *io = container_of(ev, struct bio, ev);
//   struct buf *buf = &io->sendq;
//   int n = write(ev->fd, buf_head(buf), buf_len(buf));
//   buf->head += n;
//   return 0;
// }

static inline void buf_reset(struct buf *buf, void *p, size_t cap) {
  buf->head = buf->tail = 0;
  buf->cap = cap;
  buf->ptr = p;
  *buf_tail(buf) = 0;
}

ssize_t bio_write(struct bio *io, const char *p, size_t size) {
  struct buf *buf = &io->sendq;
  size_t l = buf_awail(buf), n;
  if (l <= 0)
    return 0;
  if (l < size) {
    memcpy(buf->ptr, buf_head(buf), buf_len(buf));
    buf->tail -= buf->head;
    l = buf_awail(buf);
  }
  n = (size < l) ? size : l;
  memcpy(buf_tail(buf), p, n);
  return n;
}

ssize_t bio_flush(struct bio *io) {
  struct buf *buf = &io->sendq;
  ssize_t w, n = 0, len = buf_len(buf);
  while (n < len) {
    w = write(io->ev.fd, buf_head(buf) + n, len - n);
    if (w < 0)
      return w;
    else if (w == 0) {
      if (io->close)
        io->close(io);
      return 0;
    }
    n += w;
  }
  buf_reset(buf, buf->ptr, buf->cap);
  return n;
}

struct bio *bio_alloc(struct loop *L, int fd, int nr, int nw,  //
                      __bio_read __recv, __bio_close __close) {
  struct bio *io;
  if (!(io = xalloc(NULL, sizeof(*io) + nr + nw)))
    return NULL;
  io->ev.fd = fd;
  io->ev.events = EV_READ;
  io->ev.callback = on_read;
  if (loop_add(L, &io->ev) < 0)
    goto err;
  assert(__recv);
  io->read = __recv;
  io->close = __close;
  buf_reset(&io->recvq, ((char *)io) + sizeof(*io), nr);
  buf_reset(&io->sendq, ((char *)io) + sizeof(*io) + nr, nw);
  return io;
err:
  xfree(io);
  return NULL;
}

void bio_free(struct bio *io) { xfree(io); }
