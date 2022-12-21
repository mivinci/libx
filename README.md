# libx

A low-level C library for network programming.

## Goals

- Avoid high-level encapsulation where a socket file descriptor is usually wrapped in a structure, so that the socket file descriptor can be used directly with system calls like `send`, `select`, `fcntl`, etc.
- Compatiable with both IPv4 and IPv6.
- Compatiable with Linux (>=2.6.27) and macOS.
- Interfaces for error handling and logging.

## Installation

```
make
make install
```

## APIs

Include the needed header file.

```c
#include <x/net.h>
```



### UDP

Bind a UDP port with an IPv4 address.

```c
int sockfd = udp_bind("127.0.0.1", 8080);
```

or on all network interfaces.

```c
int sockfd = udp_bind(NULL, 8080);
```

Connect a local UDP socket to a remote one (usually on client side) so that you can use `recv` or `send` system calls to transmit data from or to the other end. Otherwise, you have to use `recvfrom` and `sendto` each time with a specific address.

```c
int err = udp_connect("8.8.8.8", 53);
```

or to a domain name.

```c
int err = udp_connect("example.com", 53);
```

### TCP

Listen a TCP port with an IPv4 address.

```c
int sockfd = tcp_listen("127.0.0.1", 8000);
```

similarly or on all network interfaces.

```c
int sockfd = tcp_listen(NULL, 8000);
```

Accept connections from a socket.

```c
struct sockaddr_storage peer;
int peerfd = tcp_accept(sockfd, &peer);
```

where `peer` and `peerfd` are the address and socket file descriptor of the other end that opened the connection.

Then you may need to convert `peer` into a version-specific address if you are familiar with C programming.

```c
struct sockaddr_in  *v4 = (struct sockaddr_in*)&peer;
struct sockaddr_in6 *v6 = (struct sockaddr_in6*)&peer;
```

Connect to a remote TCP address.

```c
int sockfd = tcp_connect("8.8.8.8", 8000);
```

or to a domain name.

```c
int sockfd = tcp_connect("example.com", 8000);
```

### UNIX Domain Socket

Bind a local file with a socket type specified.

```c
// for UDP
int sockfd = unix_bind("/tmp/example.sock", SOCK_DGRAM);
// for TCP
int sockfd = unix_bind("/tmp/example.sock", SOCK_STREAM);
```

Connect to a local file with a socket type specified.

```c
// for UDP
int sockfd = unix_connect("/tmp/example.sock", SOCK_DGRAM);
// for TCP
int sockfd = unix_connect("/tmp/example.sock", SOCK_STREAM);
```

Bind and listen for TCP connections from a UNIX domain socket. Note that you don't have to call `unix_bind` before using `unix_listen` which binds the given file name with a socket type `SOCK_STREAM` behind the scene.

```c
int sockfd = unix_listen("/tmp/example.sock");
```

Accept connections from a UNIX domain socket.

```c
struct sockaddr_un peer;
int peerfd = unix_accept(sockfd, &peer);
```

where again `peer` and `peerfd` are the address and socket file descriptor of the other end that opened the connection.

### Utilities

Make a socket file descriptor (non-)blocking.

```c
set_nonblocking(sockfd, 1);
```

or 0 for blocking socket.

Here're also APIs for reading or writing exactly the same size as you expect. Note that the first two are for TCP, UDP or UNIX domain sockets, and the last two are for UDP sockets only.

```c
ssize_t sock_read(int, char*, size_t);
ssize_t sock_write(int, const char*, size_t);
ssize_t sock_readfrom(int, char*, size_t, struct sockaddr_storage*);
ssize_t sock_writeto(int, const char*, size_t, struct sockaddr_storage*);
```

### TUN Device

Open a TUN device. ([What is a TUN device?](https://en.wikipedia.org/wiki/TUN/TAP))

```c
int tunfd = tun_open("/dev/net/tun");
```

then use the following APIs to read/write from/to the TUN device. Note that like `recv` and `send` system calls, it is not guaranteed that they read/write exactly the same size as you expect.

```c
ssize_t tun_read(int, char*, size_t);
ssize_t tun_write(int, const char*, size_t);
```

## Todos

- Encapsulate an event loop
- Add examples for every APIs possible


## License

This library is unlicense licensed :)