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

Connect a local UDP socket to a remote one (usually on client side) so that you can use `recv` and `send` to transmit data from and to the other end. Otherwise, you have use `recvfrom` and `sendto` each time with a specific address.

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

## Todos

- Encapsulate an event loop
