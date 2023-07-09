## APIs

Here's a brief introduction to APIs provided by `net.h`, `ev.h` and `co.h`. Check out [example](./example/) for details.

### net.h

Include the needed header file.

```c
#include <x/net.h>
```

#### UDP

Bind a UDP port with an IPv4 address.

```c
int sockfd = udp_bind("127.0.0.1", 8080);
```

or on all network interfaces.

```c
int sockfd = udp_bind(NULL, 8080);
```

Connect a local UDP socket to a remote one so that you can use `recv` or `send` system calls to transmit data from or to the other end,  otherwise, you have to use `recvfrom` and `sendto` each time with a specific address.

```c
int err = udp_connect(sockfd, "8.8.8.8", 53);
```

or to a domain name.

```c
int err = udp_connect(sockfd, "example.com", 53);
```

#### TCP

Bind and listen on a TCP port with an IPv4 address.

```c
int sockfd = tcp_listen("127.0.0.1", 8000);
```

or on all network interfaces.

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

#### UNIX Domain Socket

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

#### Utilities

To make a socket file descriptor (non-)blocking, call `set_blocking` that takes two arguments - a file descriptor and (0)1.

```c
void set_blocking(int, int);
```

Here're also functions to read or write exactly N bytes to a socket. Note that the first two are for TCP, UDP or UNIX domain sockets, and the last two are for UDP sockets only.

```c
ssize_t sock_read(int, char*, size_t);
ssize_t sock_write(int, const char*, size_t);
ssize_t sock_readfrom(int, char*, size_t, struct sockaddr_storage*);
ssize_t sock_writeto(int, const char*, size_t, struct sockaddr_storage*);
```

#### TUN Device

Wait! [What is a TUN device?](https://en.wikipedia.org/wiki/TUN/TAP)

To open a TUN device, call `tun_open` that takes an argument representing a TUN interface name or will be assigned by the kernel in case of an empty name,  

```c
int tun_open(char *);
```

then use the following functions to read/write from/to the TUN device. Note that like `recv` and `send` system calls, it is not guaranteed that they read/write exactly N bytes as you expect.

```c
ssize_t tun_read(int, char*, size_t);
ssize_t tun_write(int, const char*, size_t);
```

### ev.h

Include the needed header file.

```c
#include <x/ev.h>
```

To create an event loop instance, call `loop_new` that takes an argument as a hint for the event loop to the number of events to be added.

```c
struct loop *loop_new(int);
```

To operate on an event loop, call `loop_ctl`

```c
int loop_ctl(struct loop*, int op, struct ev*);
```

where `op` can be one of

- EV_CTL_ADD
- EV_CTL_MOD
- EV_CTL_DEL

to add, modify and delete an event, or use the macros defined for the three operations.

```c
loop_add(L, ev)
loop_mod(L, ev)
loop_del(L, ev)
```

To start an event loop, call `loop_sched` that blocks on dispatching fired events to their callback functions, and returns the number of dispatched events if the event loop is stopped.

```c
int loop_sched(struct loop*);
```

Finally drop the event loop by calling `loop_free` as y'all expected.

```c
void loop_free(struct loop*);
```

Unlike any other famous event loop implementations, there's only one event representaion in libx. It is simple and can be used as either an IO event or a timeout event, or both, depending on what values are given to the event. Here's the definition of an event.

```c
struct ev {
  int fd;
  unsigned events;
  long long ms;
  void (*callback)(struct loop *loop, int revents, struct ev *ev);
  ... // some members used only by the event loop this event is registered on.
};
```

where `fd` should be given a value in case of an IO event and `ms` in case of an timeout event. `events` should be given one or more values from 

- EV_READ
- EV_WRITE
- EV_TIMEOUT

according to what kind of event you want it to be. And `callback` is a pointer to a function that is called by the event loop when the corresponding event is fired.

A callback function takes three arguments where `loop` points to the event loop that calls the callback function, `revents` is the events fired on `ev`. You may take different actions according to `revents` in the callback function as follows.

```c
void callback(struct loop *loop, int revents, struct ev *ev) {
  if (revents & EV_READ) { /** do something **/ }
  if (revents & EV_WRITE) { /** do something **/ }
  if (revents & EV_TIMEOUT) { /** do something **/ }
}
```

### co.h

work in progress ...

