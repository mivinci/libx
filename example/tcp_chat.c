#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../net.h"

#define ADDR_MAX 16
#define BUF_MAX  32
#define PORT     55555

void usage(const char *prog) {
  fprintf(stdout,
          "usage: %s [args] [opts]\n"
          "  -l IP   specify an IP address\n"
          "  -p port specify a port\n"
          "  -h      print this help message\n",
          prog);
}

int main(int argc, char **argv) {
  int opt, port = PORT;
  char addr[ADDR_MAX];
  memset(addr, 0, sizeof addr);

  while ((opt = getopt(argc, argv, "l:p:h")) > 0) {
    switch (opt) {
    case 'l':
      strncpy(addr, optarg, ADDR_MAX - 1);
      break;
    case 'p':
      port = atoi(optarg);
      break;
    case 'h':
      usage(argv[0]);
      exit(0);
    }
  }

  int sockfd;
  if ((sockfd = tcp_listen(addr, port)) < 0) {
    exit(1);
  }

  int peerfd;
  struct sockaddr_storage peer;
  char buf[BUF_MAX];

  fd_set all, rfds;
  FD_ZERO(&all);
  FD_SET(sockfd, &all);

  int maxfd = sockfd;

  while (1) {
    rfds = all;
    if (select(maxfd + 1, &rfds, NULL, NULL, NULL) < 0) {
      perror("select");
      exit(1);
    }

    for (int i = 0; i <= maxfd; ++i) {
      if (FD_ISSET(i, &rfds)) {
        if (i == sockfd) {
          if ((peerfd = tcp_accept(sockfd, &peer)) < 0) {
            perror("accept");
            exit(1);
          }
          FD_SET(peerfd, &all);
          if (peerfd > maxfd) {
            maxfd = peerfd;
          }
        } else {
          int nread = recv(i, buf, BUF_MAX - 1, 0);
          if (nread < 0) {
            perror("read");
            continue;
          } else if (nread == 0) {
            close(i);
            FD_CLR(i, &all);
            continue;
          }
          for (int j = 0; j <= maxfd; ++j) {
            if (FD_ISSET(j, &all)) {
              if (j == sockfd) {
                continue;
              }
              if (send(j, buf, nread, 0) < 0) {
                perror("send");
              }
            }
          }
        }
      }
    }
  }
  return 0;
}