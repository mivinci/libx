#include <arpa/inet.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include "../net.h"

#define IFACE_STRLEN 16
#define BUF_SIZE     1024

int main(int argc, char **argv) {
  char iface[IFACE_STRLEN] = "";
  char local[INET_ADDRSTRLEN], remote[INET_ADDRSTRLEN];
  unsigned short port;
  int netfd, tunfd, maxfd;
  int opt, err;
  int has_remote = 0;

  while ((opt = getopt(argc, argv, "i:r:l:p:")) > 0) {
    switch (opt) {
    case 'p':
      port = atoi(optarg);
      break;
    case 'r':
      memcpy(remote, optarg, INET_ADDRSTRLEN);
      has_remote = 1;
      break;
    case 'l':
      memcpy(local, optarg, INET_ADDRSTRLEN);
      break;
    case 'i':
      memcpy(iface, optarg, IFACE_STRLEN);
      break;
    default:
      return EXIT_FAILURE;
    }
  }
  netfd = udp_bind(local, port);
  assert(netfd > 0);
  if (has_remote) {
    printf("connect to %s:%u\n", remote, port);
    err = udp_connect(netfd, remote, port);
    assert(err == 0);
  }
  tunfd = tun_open(iface);
  assert(tunfd > 0);

  maxfd = tunfd > netfd ? tunfd : netfd;

  fd_set fds, _fds;
  FD_ZERO(&fds);
  FD_SET(tunfd, &fds);
  FD_SET(netfd, &fds);

  int nevents;
  ssize_t n;
  char buf[BUF_SIZE];

  for (;;) {
    _fds = fds;
    nevents = select(maxfd, &_fds, NULL, NULL, NULL);
    assert(nevents > 0);

    if (FD_ISSET(netfd, &_fds)) {
      n = sock_read(netfd, buf, BUF_SIZE);
      tun_write(tunfd, buf, n);
      continue;
    }

    if (FD_ISSET(tunfd, &_fds)) {
      n = tun_read(tunfd, buf, BUF_SIZE);
      sock_write(netfd, buf, n);
      continue;
    }
  }
  return EXIT_SUCCESS;
}