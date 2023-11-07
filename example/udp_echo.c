#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../ev.h"
#include "../net.h"

#define BUFMAX 64

int c(struct loop *loop, struct ev *ev) {
  struct sockaddr_in sa;
  socklen_t len = sizeof(sa);
  ssize_t n;
  char buf[BUFMAX];

  int fd = ev->fd;
  n = recvfrom(fd, buf, BUFMAX, 0, (struct sockaddr *)&sa, &len);
  if (n < 0) {
    if (errno == EINTR)
      return 0;
    perror("recvfrom");
    exit(1);
  }

  char addr[16];
  inet_ntop(AF_INET, &sa, addr, len);
  printf("received %luB from %s:%u\n", n, addr, sa.sin_port);

  n = sendto(fd, buf, n, 0, (struct sockaddr *)&sa, len);
  if (n < 0) {
    if (errno == EINTR)
      return 0;
    perror("sendto");
    exit(1);
  }
  printf("echoed %luB\n", n);
  return 0;
}

int main(int argc, char **argv) {
  struct loop *L;
  int fd, port;

  if (argc != 2) {
    fprintf(stderr, "Usage: %s port\n", argv[0]);
    return 1;
  }

  port = atoi(argv[1]);
  assert(port > 0);

  fd = udp_bind(NULL, 8080);
  assert(fd > 0);

  L = loop_alloc(1);
  assert(L);

  struct ev e;
  e.fd = fd;
  e.events = EV_READ;
  e.callback = c;

  loop_add(L, &e);

  loop_wait(L);

  close(fd);

  loop_free(L);
}