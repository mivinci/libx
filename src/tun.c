#include <errno.h>
#include <fcntl.h>
#include <netinet/ip.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

#include "x/net.h"

#ifdef __linux__
#include <linux/if.h>
#include <linux/if_tun.h>

int tun_open(char *dev) {
  struct ifreq ifr;
  int fd;

  if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
    perror("open /dev/net/tun");
    return fd;
  }
  memset(&ifr, 0, sizeof(ifr));

  ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
  if (*dev)
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);

  int err;
  if ((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0) {
    perror("ioctl(TUNSETIFF)");
    close(fd);
    return err;
  }

  if (!*dev)
    strcpy(dev, ifr.ifr_name);
  return fd;
}

ssize_t tun_read(int fd, char *buf, size_t n) { return read(fd, buf, n); }

ssize_t tun_write(int fd, const char *buf, size_t n) {
  return write(fd, buf, n);
}
#endif  // __linux__

#ifdef __APPLE__
#include <net/if_utun.h>
#include <sys/kern_control.h>
#include <sys/sys_domain.h>  // SYSPROTO_CONTROL

#define IFNAMSIZ 16

#define UTUN_UNSUPPORTED -2
#define UTUN_INUSE       -1

static int utun_open(struct ctl_info ci, int utun_num) {
  struct sockaddr_ctl sc;
  int fd;

  if ((fd = socket(AF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL)) < 0) {
    return UTUN_UNSUPPORTED;
  }

  if (ioctl(fd, CTLIOCGINFO, &ci) == -1) {
    close(fd);
    return UTUN_UNSUPPORTED;
  }

  sc.sc_id = ci.ctl_id;
  sc.sc_len = sizeof(sc);
  sc.sc_family = AF_SYSTEM;
  sc.ss_sysaddr = AF_SYS_CONTROL;
  sc.sc_unit = utun_num + 1;

  // if the connect is successful, a utun%d device will be created, where "%d"
  // is (sc.sc_unit - 1)
  if (connect(fd, (struct sockaddr *)&sc, sizeof(sc)) < 0) {
    close(fd);
    return UTUN_INUSE;
  }
  // TODO: set non-blocking?
  return fd;
}

int tun_open(char *dev) {
  struct ctl_info ci;
  int utun_num = -1;
  int fd;

  if (*dev != 0 && strcmp("utun", dev) != 0) {  // not equal
    if (sscanf(dev, "utun%d", &utun_num) != 1) {
      // zero or more than one integer found
      // TODO: log utun name should be utunX where X is a device number
      return -1;
    }
  }

  size_t ctl_name_len = sizeof(ci.ctl_name);
  if (strlcpy(ci.ctl_name, UTUN_CONTROL_NAME, ctl_name_len) >= ctl_name_len) {
    perror("UTUN_CONTROL_NAME too long");
    return -1;
  }

  // if no utun name was specified, find the first one available
  if (utun_num != -1) {
    fd = utun_open(ci, utun_num);
  } else {
    for (utun_num = 0; utun_num < 255; ++utun_num) {
      if ((fd = utun_open(ci, utun_num)) != UTUN_INUSE) {
        break;
      }
    }
  }

  if (fd < 0) {
    return fd;
  }

  socklen_t utun_name_len = sizeof(dev);
  if (getsockopt(fd, SYSPROTO_CONTROL, UTUN_OPT_IFNAME, dev, &utun_name_len) <
      0) {
    return -1;
  }
  return fd;
}

ssize_t tun_read(int fd, char *buf, size_t n) {
  uint32_t pi;
  struct iovec iv[2];

  iv[0].iov_base = &pi;
  iv[0].iov_len = sizeof(pi);
  iv[1].iov_base = (void *)buf;
  iv[1].iov_len = n;

  ssize_t no_pi = readv(fd, iv, 2) - sizeof(pi);  // strip packet info
  return no_pi > 0 ? no_pi : 0;
}

ssize_t tun_write(int fd, const char *buf, size_t n) {
  uint32_t pi;
  struct iovec iv[2];
  struct ip *iph;

  iph = (struct ip *)buf;  // this is awesome!
  pi = iph->ip_v == 6 ? htonl(AF_INET6) : htonl(AF_INET);

  iv[0].iov_base = &pi;
  iv[0].iov_len = sizeof(pi);
  iv[1].iov_base = (void *)buf;
  iv[1].iov_len = n;

  ssize_t no_pi = writev(fd, iv, 2) - sizeof(pi);  // strip packet info
  return no_pi > 0 ? no_pi : 0;
}
#endif  // __APPLE__

#if TEST_TUN_READ

// On Linux (sudo when needed)
//   ip tuntap add mode tun
//   ip link set tun0 up
//   ip addr add 10.0.0.1/24 dev tun0

// On macOS (sudo when needed)
//

// cc -D TEST_TUN_READ -o tun tun.c && ./tun

#include <assert.h>
#include <unistd.h>

static void println_hex(const char *buf, int n) {
  for (int i = 0; i < n; ++i)
    fprintf(stdout, "%02x ", buf[i]);
  fputc('\n', stdout);
}

int main(void) {
  int tunfd;
  int nread, nwrite;
  char buf[32];

  tunfd = tun_open("tun0");
  assert(tunfd > 0);

  nread = tun_read(tunfd, buf, 32);
  assert(nread > 0);

  println_hex(buf, nread);

  nwrite = tun_write(tunfd, buf, nread);
  assert(nwrite > 0);

  close(tunfd);
  return 0;
}

#endif
