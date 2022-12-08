#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/ip.h>

#include "net.h"
#include "log.h"

#define DEF_PORT 55555

#ifdef __linux__
int tun_alloc(char *dev) {
    struct ifreq ifr;
    int fd;

    if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
        perror("open /dev/net/tun");
        return fd;
    }
    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_flags = IFF_TUN|IFF_NO_PI;
    if (*dev) {
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    }

    if( (err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0 ) {
        perror("ioctl(TUNSETIFF)");
        close(fd);
        return err;
    }
    strcpy(dev, ifr.ifr_name);
    return fd;
}
#endif

#ifdef __APPLE__
#include <sys/ioctl.h>
#include <sys/kern_control.h>
#include <sys/sys_domain.h> // SYSPROTO_CONTROL
#include <net/if_utun.h>

#define IFNAMSIZ 16

#define UTUN_UNSUPPORTED -2
#define UTUN_INUSE -1

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
    sc.sc_unit = utun_num+1;

    // if the connect is successful, a utun%d device will be created, where "%d"
    // is (sc.sc_unit - 1)
    if (connect(fd, (struct sockaddr*)&sc, sizeof(sc)) < 0) {
        close(fd);
        return UTUN_INUSE;
    }
    // todo: set non-blocking?
    return fd;
}

int tun_alloc(char *dev) {
    struct ctl_info ci;
    int utun_num = -1;
    int fd;

    if (*dev != 0 && strcmp("utun", dev) != 0) { // not equal
        if (sscanf(dev, "utun%d", &utun_num) != 1) { // zero or more than one integer found
            errorf("utun name should be utunX where X is a device number, but got %s", dev);
            return -1;
        }
    }

    size_t ctl_name_len = sizeof(ci.ctl_name);
    if (strlcpy(ci.ctl_name, UTUN_CONTROL_NAME, ctl_name_len) >= ctl_name_len) {
        errorf("UTUN_CONTROL_NAME too long");
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
    if (getsockopt(fd, SYSPROTO_CONTROL, UTUN_OPT_IFNAME, dev, &utun_name_len) < 0) {
        return -1;
    }
    return fd;
}

ssize_t tun_read(int fd, char *buf, size_t n) {
    uint32_t pi;
    struct iovec iv[2];

    iv[0].iov_base = &pi;
    iv[0].iov_len = sizeof(pi);
    iv[1].iov_base = buf;
    iv[1].iov_len = n;

    ssize_t no_pi = readv(fd, iv, 2) - sizeof(pi); // strip packet info
    return no_pi > 0? no_pi : 0;
}

ssize_t tun_write(int fd, char *buf, size_t n) {
    uint32_t pi;
    struct iovec iv[2];
    struct ip *iph;

    iph = (struct ip*)buf; // this is awesome!
    pi = iph->ip_v == 6? htonl(AF_INET6) : htonl(AF_INET);

    iv[0].iov_base = &pi;
    iv[0].iov_len = sizeof(pi);
    iv[1].iov_base = buf;
    iv[1].iov_len = n;

    ssize_t no_pi = writev(fd, iv, 2) - sizeof(pi); // strip packet info
    return no_pi > 0? no_pi : 0;
}
#endif

#if 0 // test

static void usage(const char *prog) {
    fprintf(stderr,
        "usage: %s [options]\n"
        "  -i dev  specify a tun device name\n"
        "  -p port specify a port (default: %d)\n"
        , prog, DEF_PORT);
}

static void debug_hex(const char *buf, int n) {
    for (int i = 0; i < n; ++i) {
        fprintf(stdout, "%02x ", buf[i]);
    }
    fputc('\n', stdout);
}

int main(int argc, char **argv)
{
    int tun_fd, inet_fd;
    int opt;
    int port = DEF_PORT;
    char dev[IFNAMSIZ];

    memset(dev, 0, sizeof(dev));

    while ((opt = getopt(argc, argv, "i:p:v")) > 0) {
        switch (opt) {
        case 'v':
            xlogf_setlevel(LF_INFO);
            break;
        case 'i':
            strncpy(dev, optarg, IFNAMSIZ-1);
            break;
        case 'p':
            port = atoi(optarg);
            break;
        default:
            usage(argv[0]);
            exit(1);
        }
    }

    if ((tun_fd = tun_alloc(dev)) < 0) {
        debug("open tun: %s", strerror(errno));
        exit(1);
    }
    
    char buf[128];
    memset(buf, 0, 128);

    for (;;) {
        int n_read = tun_read(tun_fd, buf, 128);
        printf("read %uB\n", n_read);
        debug_hex(buf, n_read);
    }

    return 0;
}

#endif
