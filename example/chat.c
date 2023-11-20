#include <arpa/inet.h>
#include <assert.h>
#include <getopt.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "x/ev.h"
#include "x/list.h"
#include "x/mm.h"
#include "x/net.h"

#define FREELIST_MAX 32
#define BUF_MAX      1024

struct server {
  struct loop *L;
  struct ev ev;
  struct list_head conns;
  struct list_head freelist;
  int len;
};

struct tcp_conn {
  struct list_head node;
  struct sockaddr_in sa;
  socklen_t sa_size;
  struct ev ev;
  struct server *S;   // weak pointer
  char buf[BUF_MAX];  // read buffer
};

static struct tcp_conn *getconn(struct server *S) {
  struct tcp_conn *C;
  struct list_head *p;
  if (S->len <= 0) {
    if (!(C = xalloc(NULL, sizeof(*C))))
      return NULL;
    memset(C, 0, sizeof(*C));
    C->sa_size = sizeof(C->sa);
    return C;
  }
  p = S->freelist.next;
  list_del(p);
  S->len--;
  C = container_of(p, struct tcp_conn, node);
  assert(C);
  return C;
}

static void putconn(struct server *S, struct tcp_conn *C) {
  struct tcp_conn *p;
  if (S->len >= FREELIST_MAX)
    xalloc(C, 0);
  else {
    list_add(&C->node, &S->freelist);
    S->len++;
  }
}

static int broadcast(struct server *S, const char *buf, size_t n) {
  struct tcp_conn *C;
  struct list_head *p;
  list_foreach(p, &S->conns) {
    C = container_of(p, struct tcp_conn, node);
    assert(C);
    write(C->ev.fd, buf, n);  // we simply write for now.
  }
  return 0;
}

int on_recv_server(struct loop *L, struct ev *ev) {
  struct tcp_conn *C;
  ssize_t n;
  C = container_of(ev, struct tcp_conn, ev);
  assert(C);
  if (ev->revents & EV_TIMER)
    goto _close;
  if ((n = read(ev->fd, C->buf, BUF_MAX)) <= 0)
    goto _close;
  return broadcast(C->S, C->buf, n);
_close:
  close(ev->fd);
  list_del(&C->node);
  putconn(C->S, C);
  return 0;
}

int on_accept(struct loop *L, struct ev *ev) {
  struct server *S;
  struct tcp_conn *C;
  int fd;
  S = container_of(ev, struct server, ev);
  assert(S);
  C = getconn(S);
  assert(C);
  if ((fd = tcp_accept(ev->fd, (struct sockaddr *)&C->sa, &C->sa_size)) < 0)
    return fd;
  C->ev.fd = fd;
  C->ev.events = EV_READ;
  C->ev.callback = on_recv_server;
  C->S = S;
  list_add(&C->node, &S->conns);
  loop_add(L, &C->ev);
  return 0;
}

void server_init(struct server *S, const char *host, unsigned short port) {
  S->L = loop_alloc(32);
  assert(S->L);
  S->ev.fd = tcp_listen(host, port);
  assert(S->ev.fd);
  S->ev.events = EV_READ;
  S->ev.callback = on_accept;
  S->len = 0;
  list_head_init(&S->freelist);
  list_head_init(&S->conns);
  loop_add(S->L, &S->ev);
}

void server_close(struct server *S) {
  close(S->ev.fd);  // close listenfd
  loop_free(S->L);
}

int server_run(struct server *S) { return loop_wait(S->L); }

int run_server(const char *host, unsigned short port) {
  struct server S;
  int err;
  server_init(&S, host, port);
  err = server_run(&S);
  server_close(&S);
  return err;
}

struct client {
  struct loop *L;
  struct ev ev_stdin;
  struct ev ev_net;
  char buf[BUF_MAX];  // read buffer
};

int on_recv_client(struct loop *L, struct ev *ev) {
  int n;
  struct client *C = container_of(ev, struct client, ev_net);
  assert(C);
  n = read(ev->fd, C->buf, BUF_MAX);
  if (n == 0)   // connection closed by server
    return -1;  // tell our event loop to exit
  else if (n < 0) {
    perror("read(net)");
    return n;
  }
  if ((n = write(STDOUT_FILENO, C->buf, n)) < 0) {
    perror("write(stdout)");
    return n;
  }
  return 0;
}

int on_recv_client_stdin(struct loop *L, struct ev *ev) {
  assert(ev->revents == EV_READ);
  int n;
  struct client *C = container_of(ev, struct client, ev_stdin);
  assert(C);
  if ((n = read(ev->fd, C->buf, BUF_MAX)) < 0) {
    perror("read(stdin)");
    return n;
  }
  if ((n = write(C->ev_net.fd, C->buf, n)) < 0) {
    perror("write(net)");
    return n;
  }
  return 0;
}

void client_init(struct client *C, const char *host, unsigned short port) {
  C->L = loop_alloc(2);
  assert(C->L);
  C->ev_net.fd = tcp_connect(host, port);
  assert(C->ev_net.fd);
  C->ev_net.events = EV_READ;
  C->ev_net.callback = on_recv_client;
  loop_add(C->L, &C->ev_net);
  C->ev_stdin.fd = STDIN_FILENO;
  C->ev_stdin.events = EV_READ;
  C->ev_stdin.callback = on_recv_client_stdin;
  loop_add(C->L, &C->ev_stdin);
}

void client_close(struct client *C) {
  close(C->ev_net.fd);
  loop_free(C->L);
}

int client_run(struct client *C) { return loop_wait(C->L); }

int run_client(const char *host, unsigned short port) {
  struct client C;
  int err;
  client_init(&C, host, port);
  err = client_run(&C);
  client_close(&C);
  return err;
}

void usage(const char *argv0) {
  fprintf(stdout,
          "usage: %s [options] host port\n"
          "options:\n"
          "  -h    display this help and exit\n"
          "  -c    client mode (default)\n"
          "  -s    server mode\n",
          argv0);
}

int server_mode = 0;

int main(int argc, char **argv) {
  const char *host;
  unsigned short port;
  int opt, err;

  while ((opt = getopt(argc, argv, "sch")) > 0) {
    switch (opt) {
    case 'c':
      break;
    case 's':
      server_mode = 1;
      break;
    case 'h':
      usage(argv[0]);
      return 0;
    }
  }

  host = argv[optind];
  port = atoi(argv[++optind]);
  err = server_mode ? run_server(host, port) : run_client(host, port);
  return err;
}
