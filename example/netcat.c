#include <assert.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "x/ev.h"
#include "x/net.h"

int listen_mode = 0;
int udp_mode = 0;

void usage(const char *argv0) {
  fprintf(stdout,
          "usage: %s [options] host port\n"
          "options:\n"
          "  -h    display this help and exit\n"
          "  -l    listen mode, for inbound connects\n"
          "  -u    UDP mode\n",
          argv0);
}

int on_inbound(struct loop *L, struct ev *ev) {
  assert(ev->revents == EV_READ);
  struct sockaddr_storage sa;
  char buf[1024];
  int n;
  if (udp_mode) {
    if ((n = sock_readfrom(ev->fd, buf, 1024, &sa)) < 0)
      return n;
    return sock_writeto(ev->fd, buf, 1024, &sa);
  }
  // TODO: tcp accept
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
  int peerfd = *(int*)ev->ud, n;
  char buf[1024];
  if ((n = sock_read(STDIN_FILENO, buf, 1024)) < 0)
    return n;
  return sock_write(peerfd, buf, n);
}

int outbound(struct loop *L, const char *host, unsigned short port) {
  struct ev ev;
  int fd, peerfd;
  if (!udp_mode)
    peerfd = tcp_connect(host, port);
  else {
    fd = udp_bind(NULL, 0);
    assert(fd);
    peerfd = udp_connect(fd, host, port);
  }
  assert(peerfd);
  ev.events = EV_READ;
  ev.callback = on_outbound;
  ev.fd = STDIN_FILENO;
  ev.ud = &peerfd;
  loop_add(L, &ev);
  loop_wait(L);
  close(peerfd);
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
    case 'h':
      usage(argv[0]);
      return 0;
    }
  }

  host = argv[optind];
  port = atoi(argv[++optind]);
  L = loop_alloc(16);
  assert(L);
  err = listen_mode ? inbound(L, host, port)
                    : outbound(L, host, port);

  loop_free(L);
  return err;
}
