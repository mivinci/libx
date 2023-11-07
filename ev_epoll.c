#include <sys/epoll.h>

struct state {
  int epfd;
  struct epoll_event *events;
};

static int api_init(struct loop *loop) {
  struct state *state;

  state = xalloc(NULL, sizeof(*state));
  if (unlikely(!state))
    goto err;

  state->events = xalloc(NULL, sizeof(struct epoll_event) * loop->cap);
  if (unlikely(!state->events))
    goto err;

  state->epfd = epoll_create(1024);
  if (state->epfd < 0)
    goto err;

  loop->state = state;
  return 0;

err:
  if (state->events)
    xalloc(state->events, 0);
  if (state)
    xalloc(state, 0);
  return -1;
}

static void api_free(struct loop *loop) {
  struct state *state = loop->state;
  close(state->epfd);
  xalloc(state->events, 0);
  xalloc(state, 0);
}

static int api_realloc(struct loop *loop, int cap) {
  struct state *state = loop->state;
  state->events = xalloc(state->events, sizeof(struct epoll_event) * cap);
  if (unlikely(!state->events))
    return -1;
  return 0;
}

static int api_ctl(struct loop *loop, int op, int fd, int events) {
  struct state *state = loop->state;
  struct epoll_event ev;

  switch (op) {
  case EV_CTL_ADD:
    op = EPOLL_CTL_ADD;
    break;
  case EV_CTL_MOD:
    op = EPOLL_CTL_MOD;
    break;
  case EV_CTL_DEL:
    op = EPOLL_CTL_DEL;
    break;
  default:
    op = -1;
  }
  if (op < 0)
    return -2;

  ev.data.fd = fd;
  ev.events = 0;
  if (events & EV_READ)
    ev.events |= EPOLLIN;
  if (events & EV_WRITE)
    ev.events |= EPOLLOUT;
  if (epoll_ctl(state->epfd, op, fd, &ev) < 0)
    return -2;
  return 0;
}

static int api_poll(struct loop *loop, struct timeval *tv) {
  struct state *state = loop->state;
  struct epoll_event *ev;
  int nevents, timeout, i, events;

  timeout = tv ? tv->tv_sec * 1000 + (tv->tv_usec + 999) / 1000 : -1;
  nevents = epoll_wait(state->epfd, state->events, loop->cap, timeout);
  if (unlikely(nevents < 0 && errno != EINTR)) {
    perror("api_poll: epoll");
    exit(1);
  }
  for (i = 0; i < nevents; i++) {
    events = 0;
    ev = state->events + i;
    if (ev->events & EPOLLIN)
      events |= EV_READ;
    if (ev->events & EPOLLOUT)
      events |= EV_WRITE;
    if (ev->events & EPOLLERR)
      events |= EV_WRITE | EV_READ;
    if (ev->events & EPOLLHUP)
      events |= EV_WRITE | EV_READ;  // fd is closed.
    loop->fired[i].fd = ev->data.fd;
    loop->fired[i].events = events;
  }
  return nevents;
}
