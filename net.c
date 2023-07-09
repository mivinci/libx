#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>

#include "net.h"

static int __socket(int family, int socktype, int protocol) {
  int sockfd;

#if __linux__
  socktype |= SOCK_CLOEXEC;
#endif

  if ((sockfd = socket(family, socktype, protocol)) < 0) {
    return sockfd;
  }

#if __APPLE__
  set_cloexec(sockfd);
#endif

  return sockfd;
}

static int is_ipv6(const char *p) {
  for (; p; ++p) {
    switch (*p) {
    case ':':
      return 1;
    case '.':
      return 0;
    }
  }
  return -1;
}

static int __inet_bind(const char *addr, unsigned short port, int socktype) {
  char _port[6];
  snprintf(_port, 6, "%u", port);

  int family = is_ipv6(addr) ? AF_INET6 : AF_INET;
  if (family < 0)
    return -1;

  struct addrinfo hint, *ai, *p;
  memset(&hint, 0, sizeof hint);
  hint.ai_family = family;
  hint.ai_socktype = socktype;
  hint.ai_flags = AI_PASSIVE;  // no effect if addr != NULL

  int err;
  if ((err = getaddrinfo(addr, _port, &hint, &ai)) != 0)
    return err;

  int sockfd = -1;
  for (p = ai; p != NULL; p = p->ai_next) {
    if ((sockfd = __socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
      perror("socket");
      continue;
    }

    if (bind(sockfd, p->ai_addr, p->ai_addrlen) < 0) {
      perror("bind");
      continue;
    }
    break;
  }

  freeaddrinfo(ai);
  return sockfd;
}

static int __connect(int sockfd, const char *addr, unsigned short port,
                     int socktype) {
  char _port[6];
  snprintf(_port, 6, "%u", port);

  int family = is_ipv6(addr) ? AF_INET6 : AF_INET;
  if (family < 0)
    return -1;

  struct addrinfo hint, *ai, *p;
  memset(&hint, 0, sizeof hint);
  hint.ai_family = family;
  hint.ai_socktype = socktype;

  int err;
  if ((err = getaddrinfo(addr, _port, &hint, &ai)) != 0)
    return -1;

  for (p = ai; p != NULL; p = p->ai_next) {
    if ((err = connect(sockfd, p->ai_addr, p->ai_addrlen)) < 0) {
      perror("connect");
      continue;
    }
    break;
  }

  freeaddrinfo(ai);
  return err;
}

static int __accept(int sockfd, struct sockaddr *sa, socklen_t *len) {
  int peerfd;
  for (;;) {
    if ((peerfd = accept(sockfd, sa, len)) < 0) {
      if (errno == EINTR)
        continue;
      perror("accept");
      return peerfd;
    }
    break;
  }
  set_cloexec(peerfd);
  return peerfd;
}

void set_blocking(int sockfd, int blocking) {
  int old = fcntl(sockfd, F_GETFL);
  int new = blocking ? old & ~O_NONBLOCK : old | O_NONBLOCK;
  if (old != new)
    fcntl(sockfd, F_SETFL, new);
}

void set_cloexec(int sockfd) {
  int old = fcntl(sockfd, F_GETFD);
  int new = old | FD_CLOEXEC;
  if (old != new)
    fcntl(sockfd, F_SETFD, new);
}

int udp_bind(const char *addr, unsigned short port) {
  return __inet_bind(addr, port, SOCK_DGRAM);
}

int udp_connect(int sockfd, const char *addr, unsigned short port) {
  return __connect(sockfd, addr, port, SOCK_DGRAM);
}

int tcp_listen(const char *addr, unsigned short port) {
  int sockfd;
  if ((sockfd = __inet_bind(addr, port, SOCK_STREAM)) < 0) {
    return sockfd;
  }
  if (listen(sockfd, 5) < 0) {
    perror("listen");
    return -1;
  }
  return sockfd;
}

int tcp_connect(const char *addr, unsigned short port) {
  int sockfd;
  if ((sockfd = __socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket(AF_INET)");
    return -1;
  }
  return __connect(sockfd, addr, port, SOCK_STREAM);
}

int tcp_accept(int sockfd, struct sockaddr_storage *sa) {
  socklen_t len = (socklen_t)sizeof *sa;
  return __accept(sockfd, (struct sockaddr *)sa, &len);
}

static int __unix_bind(const char *path, int socktype) {
  int sockfd;
  if ((sockfd = __socket(AF_UNIX, socktype, 0)) < 0) {
    perror("socket(AF_UNIX)");
    return -1;
  }
  struct sockaddr_un sa;
  memset(&sa, 0, sizeof sa);
  sa.sun_family = AF_UNIX;
  strncpy(sa.sun_path, path, sizeof(sa.sun_path) - 1);

  if (bind(sockfd, (struct sockaddr *)&sa, sizeof sa) < 0) {
    perror("bind");
    return -1;
  }
  set_cloexec(sockfd);
  return sockfd;
}

int unix_bind(const char *path, int socktype) {
  return __unix_bind(path, socktype);
}

int unix_connect(const char *path, int socktype) {
  int sockfd;
  if ((sockfd = __socket(AF_UNIX, socktype, 0)) < 0) {
    perror("socket(AF_UNIX)");
    return -1;
  }
  struct sockaddr_un sa;
  memset(&sa, 0, sizeof sa);
  sa.sun_family = AF_UNIX;
  strncpy(sa.sun_path, path, sizeof(sa.sun_path) - 1);
  if (connect(sockfd, (struct sockaddr *)&sa, sizeof sa) < 0) {
    perror("connect");
    return -1;
  }
  return sockfd;
}

int unix_listen(const char *path) {
  int sockfd;
  if ((sockfd = __unix_bind(path, SOCK_STREAM)) < 0) {
    return sockfd;
  }
  if (listen(sockfd, 20) < 0) {
    perror("listen");
    return -1;
  }
  return sockfd;
}

int unix_accept(int sockfd, struct sockaddr_un *sa) {
  socklen_t len = (socklen_t)sizeof *sa;
  return __accept(sockfd, (struct sockaddr *)sa, &len);
}

ssize_t sock_read(int sockfd, char *buf, size_t len) {
  ssize_t total = 0, nread;
  while (total < len) {
    nread = recv(sockfd, buf, len - total, 0);
    if (nread == 0)
      return total;
    if (nread < 0)
      return -1;
    total += nread;
    buf += nread;
  }
  return total;
}

ssize_t sock_write(int sockfd, const char *buf, size_t len) {
  ssize_t total = 0, nwrite;
  while (total < len) {
    nwrite = send(sockfd, buf, len - total, 0);
    if (nwrite == 0)
      return total;
    if (nwrite < 0)
      return -1;
    total += nwrite;
    buf += nwrite;
  }
  return total;
}

ssize_t sock_readfrom(int sockfd, char *buf, size_t len,
                      struct sockaddr_storage *sa) {
  ssize_t total = 0, nread;
  size_t sa_len = sizeof *sa;
  while (total < len) {
    nread = recvfrom(sockfd, buf, len - total, 0, (struct sockaddr *)sa,
                     (socklen_t *)&sa_len);
    if (nread == 0)
      return total;
    if (nread < 0)
      return -1;
    total += nread;
    buf += nread;
  }
  return total;
}

ssize_t sock_writeto(int sockfd, const char *buf, size_t len,
                     struct sockaddr_storage *sa) {
  ssize_t total = 0, nwrite;
  while (total < len) {
    nwrite = sendto(sockfd, buf, len - total, 0, (struct sockaddr *)sa,
                    (socklen_t)sizeof *sa);
    if (nwrite == 0)
      return total;
    if (nwrite < 0)
      return -1;
    total += nwrite;
    buf += nwrite;
  }
  return total;
}

#ifdef TEST_UDP_ECHO

// cc -D TEST_UDP_ECHO -o net net.c && ./net
// nc -u 127.0.0.1 8080

#include <assert.h>
#include <unistd.h>

#include "x.h"

int main(void) {
  int sockfd;

  sockfd = udp_bind("127.0.0.1", 8080);
  assert(sockfd > 0);

  char buf[64];
  ssize_t nread, nwrite;

  for (;;) {
    struct sockaddr_storage sa;

    nread = sock_readfrom(sockfd, buf, 4, &sa);
    assert(nread == 4);

    nwrite = sock_writeto(sockfd, buf, 4, &sa);
    assert(nwrite == 4);
  }
}

#endif