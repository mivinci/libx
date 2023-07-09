#include <sys/select.h>

struct state {
  fd_set rfds;
  fd_set wfds;
};

static int api_init(struct loop *loop) {
  struct state *state;
  state = xmalloc(sizeof(*state));
  if (unlikely(!state))
    return -1;
  FD_ZERO(&state->rfds);
  FD_ZERO(&state->wfds);
  loop->state = state;
  return 0;
}

static void api_free(struct loop *loop) {
  if (loop->state)
    xfree(loop->state);
}

static int api_realloc(struct loop *loop, int cap) {
  return cap <= FD_SETSIZE ? 0 : -1;
}

static int api_ctl(struct loop *loop, int op, int fd, int events) {
  struct state *state = loop->state;
  switch (op) {
  case EV_CTL_ADD:
    if (events & EV_READ)
      FD_SET(fd, &state->rfds);
    if (events & EV_WRITE)
      FD_SET(fd, &state->wfds);
    break;
  case EV_CTL_DEL:
    if (events & EV_READ)
      FD_CLR(fd, &state->rfds);
    if (events & EV_WRITE)
      FD_CLR(fd, &state->wfds);
    break;
  default:
    return -2;
  }
  return 0;
}

static int api_poll(struct loop *loop, struct timeval *tv) {
  struct state *state = loop->state;
  struct ev *ev;
  int nevents, i, events;
  fd_set rfds, wfds;

  rfds = state->rfds;
  wfds = state->wfds;

  nevents = select(loop->maxfd, &rfds, &wfds, NULL, tv);
  if (unlikely(nevents < 0 && errno != EINTR)) {
    perror("api_poll:select");
    exit(1);
  }

  // we have to iterate over all the events added cuz it's `select` :(
  nevents = 0;
  for (i = 0; i < loop->maxfd; i++) {
    events = 0;
    ev = loop->events[i];
    if (ev->events == EV_NONE)
      continue;
    if (FD_ISSET(ev->fd, &state->rfds))
      events |= EV_READ;
    if (FD_ISSET(ev->fd, &state->wfds))
      events |= EV_WRITE;
    if (events) {
      loop->fired[nevents].fd = ev->fd;
      loop->fired[nevents].events = events;
      nevents++;
    }
  }

  return nevents;
}