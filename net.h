#ifndef _X_NET_H
#define _X_NET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>

/* UDP or TCP */
int udp_bind(const char *host, unsigned short port);
int udp_connect(int sockfd, const char *host, unsigned short port);
int tcp_listen(const char *host, unsigned short port);
int tcp_accept(int sockfd, struct sockaddr_storage *sa);
int tcp_connect(const char *host, unsigned short port);

/* UNIX Domain Socket */

int unix_bind(const char *path, int socktype);
int unix_connect(const char *path, int socktype);
int unix_listen(const char *path);
int unix_accept(int sockfd, struct sockaddr_un *sa);

/* Utility Functions */

void set_blocking(int, int);
void set_cloexec(int);
ssize_t sock_read(int, char *, size_t);
ssize_t sock_write(int, const char *, size_t);
ssize_t sock_readfrom(int, char *, size_t, struct sockaddr_storage *);
ssize_t sock_writeto(int, const char *, size_t, struct sockaddr_storage *);

/* Tun Device */

int tun_open(char *);
ssize_t tun_read(int, char *, size_t);
ssize_t tun_write(int, const char *, size_t);

#ifdef __cplusplus
}
#endif

#endif