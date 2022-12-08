#ifndef X_NET_H
#define X_NET_H

#include <stdio.h>
#include <stdbool.h>
#include <sys/un.h>
#include <sys/socket.h>

// IPv4 or IPv6
int udp_bind(const char *ip, unsigned short port);
int udp_connect(int sockfd, const char *ip, unsigned short port);
int tcp_listen(const char *ip, unsigned short port);
int tcp_accept(int sockfd, struct sockaddr_storage *sa);
int tcp_connect(const char *addr, unsigned short port);

// UNIX Domain Socket
int unix_bind(const char *path, int socktype);
int unix_connect(const char *path, int socktype);
int unix_listen(const char *path);
int unix_accept(int sockfd, struct sockaddr_un *sa);

// utility functions
void set_nonblocking(int, bool);
void set_cloexec(int);

// Tun
int tun_alloc(char*);
ssize_t tun_read(int, char*, size_t);
ssize_t tun_write(int, char*, size_t);


#endif