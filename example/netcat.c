#include <assert.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "x/ev.h"
#include "x/net.h"

#define BUF_MAX  1024
#define CONN_MAX 32

struct tcp_conn {
  struct sockaddr sa;
  socklen_t sa_size;
  struct ev ev;
};

static struct tcp_conn conns[CONN_MAX];

int listen_mode = 0;
int udp_mode = 0;

void usage(const char *argv0) {
  fprintf(stdout,
          "usage: %s [options] host port\n"
          "options:\n"
          "  -h    display this help and exit\n"
          "  -l    listen mode, for inbound connects\n"
          "  -t    TCP mode (default)\n"
          "  -u    UDP mode\n",
          argv0);
}

int on_inbound2(struct loop *L, struct ev *ev) {
  assert(ev->revents == EV_READ);
  int n;
  char buf[BUF_MAX];
  if ((n = read(ev->fd, buf, BUF_MAX)) < 0) {
    perror("read(net)");
    return n;
  }
  buf[n] = 0;
  if ((n = write(ev->fd, buf, n)) < 0) {
    perror("write(net)");
    close(ev->fd);
    ev->fd = 0;  // release the tcp connection from conns
    return n;
  }
  return 0;
}

int on_inbound(struct loop *L, struct ev *ev) {
  assert(ev->revents == EV_READ);
  int n, i;
  char buf[BUF_MAX];
  struct sockaddr sa;
  socklen_t sa_size = sizeof sa;
  if (udp_mode) {
    if ((n = recvfrom(ev->fd, buf, BUF_MAX, 0, &sa, &sa_size)) < 0) {
      perror("recvfrom(net)");
      return n;
    }
    buf[n] = 0;
    if ((n = sendto(ev->fd, buf, n, 0, &sa, sa_size)) < 0) {
      perror("sendto(net)");
      return n;
    }
  }

  struct tcp_conn *p;
  int peerfd;
  for (i = 0; i < CONN_MAX; i++) {
    p = conns + i;
    if (p->ev.fd > 0)
      continue;
    if ((peerfd = accept(ev->fd, &p->sa, &p->sa_size)) < 0) {
      perror("accept");
      return peerfd;
    }
    p->ev.fd = peerfd;
    p->ev.events = EV_READ;
    p->ev.callback = on_inbound2;
    return 0;
  }
  fprintf(stderr, "connection exceeded\n");
  return 0;
}

int inbound(struct loop *L, const char *host, unsigned short port) {
  struct ev ev;
  int fd = udp_mode ? udp_bind(host, port) : tcp_listen(host, port);
  assert(fd > 0);
  ev.events = EV_READ;
  ev.fd = fd;
  ev.callback = on_inbound;
  loop_add(L, &ev);
  loop_wait(L);
  close(fd);
  return 0;
}

int on_outbound(struct loop *L, struct ev *ev) {
  assert(ev->revents == EV_READ);
  int n;
  char buf[BUF_MAX];
  if ((n = read(STDIN_FILENO, buf, BUF_MAX)) < 0) {
    perror("read(stdin)");
    return n;
  }
  buf[n] = 0;
  if ((n = write(ev->fd, buf, n)) < 0) {
    perror("write(net)");
    return n;
  }
  return 0;
}

int outbound(struct loop *L, const char *host, unsigned short port) {
  struct ev ev;
  int fd, peerfd, err;
  if (!udp_mode)
    peerfd = tcp_connect(host, port);
  else {
    fd = udp_bind(NULL, 0);
    assert(fd);
    err = udp_connect(fd, host, port);
    assert(err == 0);
  }
  ev.events = EV_READ;
  ev.callback = on_outbound;
  ev.fd = STDIN_FILENO;
  loop_add(L, &ev);
  loop_wait(L);
  close(fd);
  return 0;
}

int main(int argc, char **argv) {
  struct loop *L;
  int err;
  const char *host;
  unsigned short port;

  int opt;
  while ((opt = getopt(argc, argv, "luh")) > 0) {
    switch (opt) {
    case 'l':
      listen_mode = 1;
      break;
    case 'u':
      udp_mode = 1;
      break;
    case 't':
      break;
    case 'h':
      usage(argv[0]);
      return 0;
    }
  }

  host = argv[optind];
  port = atoi(argv[++optind]);
  L = loop_alloc(16);
  assert(L);
  err = listen_mode ? inbound(L, host, port) : outbound(L, host, port);
  loop_free(L);
  return err;
}
