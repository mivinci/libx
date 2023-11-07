#include <assert.h>
#include <unistd.h>

#include "../ev.h"
#include "../net.h"
#include "../x.h"

#define BUFMAX 128

struct state {
  struct ev e1;
  struct ev e2;
  int sockfd;
};

int c1(struct loop *L, struct ev *ev) {
  struct state *S = container_of(ev, struct state, e1);
  char buf[BUFMAX];
  ssize_t n;
  if (ev->revents & EV_READ) {
    n = read(ev->fd, buf, BUFMAX);
    assert(n > 0);
    sock_write(S->sockfd, buf, n);
  }
  return 0;
}

int c2(struct loop *L, struct ev *ev) {
  char buf[BUFMAX];
  ssize_t n;
  if (ev->revents & EV_READ) {
    n = sock_read(ev->fd, buf, BUFMAX);
    buf[n] = 0;
    printf("%s", buf);
  }
  return 0;
}

int main(int argc, char **argv) {
  struct loop *L;
  struct state S;
  int fd, err, port;

  if (argc != 3) {
    fprintf(stderr, "Usage: %s hostname port\n", argv[0]);
    return 1;
  }

  port = atoi(argv[2]);
  assert(port > 0);

  fd = udp_bind(NULL, 0);
  assert(fd > 0);

  err = udp_connect(fd, argv[1], port);
  assert(err == 0);

  L = loop_alloc(2);
  assert(L);

  S.sockfd = fd;
  S.e1.fd = STDIN_FILENO;
  S.e1.events = EV_READ;
  S.e1.callback = c1;
  S.e2.fd = fd;
  S.e2.events = EV_READ;
  S.e2.callback = c2;

  loop_add(L, &S.e1);
  loop_add(L, &S.e2);

  loop_wait(L);
  loop_free(L);
  return 0;
}