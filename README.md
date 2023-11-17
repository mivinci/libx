# libx

A low-level C library for network programming.

## Why libx?

I always find myself writing the same old boilerplate code when doing network programming in C, but I don't need any fancy library to take over the control too much. So I'm making this low-level encapsulation over the kernel's primitives so it can be used in other projects without falling into frameworks or dependency chaos. :)

Note that this library lacks cover of tests, and there may be things I code wrong, let me know if you are interested.

## Todos

- [x] net.c - a low-level encapsulation upon the system's socket APIs.
- [x] tun.c - help interact with TUN devices on Linux (>=2.6.27) and macOS.
- [x] ev.c - a minimal event loop using epoll, kqueue, select, and a minheap for asynchronous programming. 
- [ ] co.c - a minimal stackful coroutine runtime for asynchronous programming.


## Installation

Run

```
make
```

and then copy the output file `libx.a` to whatever you want.

## Documentation

For API specifications, check out [spec.md](./docs/spec.md).

## Contribution

All commits should follow the format convention below for GitHub will translate them into emojis which's cool :)

- `:sparkles:`  introducing new features.
- `:construction:`  work in progress.
- `:memo:`  updated documentations, including README.md.
- `:bug:`  fixed a bug.

## References

- [Unix Network Programming Volume 1](https://www.amazon.com/Unix-Network-Programming-Sockets-Networking/dp/0131411551)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- [Universal TUN/TAP device driver](https://www.kernel.org/doc/html/latest/networking/tuntap.html)
- [Tun/Tap interface tutorial](https://backreference.org/2010/03/26/tuntap-interface-tutorial/index.html)
- [Rust std::net](https://github.com/rust-lang/rust)
- [Redis](https://github.com/redis/redis)
- [libevent](https://github.com/libevent/libevent)
- [libev](https://github.com/enki/libev)
- [libhv](https://github.com/ithewei/libhv)
- [mptun](https://github.com/cloudwu/mptun)

## License

libx is MIT licensed.