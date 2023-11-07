#include <assert.h>
#include <string.h>
#include <unistd.h>

#include "../ev.h"
#include "../net.h"
#include "../x.h"

#define BUF_MAX 64

struct conn {
  struct conn *next;
  struct server *S;
  struct ev ev;
  struct sockaddr_storage sa;
  char wbuf[BUF_MAX];
  unsigned nwbuf;
};

struct server {
  struct loop *loop;
  struct ev ev;
  struct conn *conns;
  unsigned nconn;
};

int cb_echo(struct loop *loop, struct ev *ev) {
  struct conn *C = container_of(ev, struct conn, ev);
  char buf[BUF_MAX];
  int nread, nwrite, sockfd = C->ev.fd;

  if (ev->revents & EV_READ) {
    nread = sock_read(sockfd, buf, BUF_MAX);
    if (nread <= 0)
      goto CLOSE;
    memcpy(C->wbuf, buf, nread);
    C->nwbuf = nread;
    return 0;
  }
  if (ev->revents & EV_WRITE) {
    nwrite = sock_write(sockfd, C->wbuf, C->nwbuf);
    if (nwrite <= 0)
      goto CLOSE;
    return 0;
  }

CLOSE:
  close(sockfd);
  memcpy(C, C->next, sizeof(struct conn));
  C->next = C->next->next;
  free(C->next);
  C->S->nconn--;
  loop_del(loop, ev);
}

int cb_accept(struct loop *loop, struct ev *ev) {
  int err;
  struct server *S = container_of(ev, struct server, ev);
  struct conn *C = malloc(sizeof(struct conn));
  memset(C, 0, sizeof(*C));
  assert(C != NULL);
  int sockfd = tcp_accept(S->ev.fd, &C->sa);
  assert(sockfd > 0);
  C->S = S;
  C->ev.fd = sockfd;
  C->ev.events = EV_READ | EV_WRITE;
  C->ev.callback = cb_echo;
  err = loop_add(loop, &C->ev);
  if (err < 0)
    goto ERR;
  C->next = S->conns;
  S->conns = C;
  S->nconn++;
  return 0;
ERR:
  close(sockfd);
  free(C);
  return 0;
}

int server_init(struct server *S, const char *ipa, unsigned short port) {
  int fd = tcp_listen(ipa, port);
  if (fd < 0)
    return fd;
  S->ev.fd = fd;
  S->ev.events = EV_READ;
  S->ev.callback = cb_accept;
  return loop_add(S->loop, &S->ev);
}

int main(void) {
  int err;
  struct server S;

  S.loop = loop_alloc(5);

  err = server_init(&S, "127.0.0.1", 8080);
  assert(err == 0);

  loop_wait(S.loop);

  loop_free(S.loop);
  return 0;
}