#ifdef __APPLE__

#include <sys/event.h>
#include <sys/time.h>

struct state {
  int kq;
  struct kevent *events;
  unsigned char *revents;  // for merging fired events by `kevent()` :(
                           // TODO: use bitset to reduce memory usage.
};

static int api_init(struct loop *loop) {
  struct state *state;

  state = xalloc(NULL, sizeof(*state));
  if (unlikely(!state))
    goto err;

  state->events = xalloc(NULL, sizeof(struct kevent) * loop->cap);
  if (unlikely(!state->events))
    goto err;

  state->revents = xalloc(NULL, sizeof(unsigned char) * loop->cap);
  if (unlikely(!state->revents))
    goto err;
  memset(state->revents, 0, sizeof(unsigned char) * loop->cap);

  state->kq = kqueue();
  if (unlikely(state->kq < 0))
    goto err;

  loop->state = state;
  return 0;

err:
  if (state->events)
    xalloc(state->events, 0);
  if (state->revents)
    xalloc(state->revents, 0);
  if (state)
    xalloc(state, 0);
  return -1;
}

static void api_free(struct loop *loop) {
  struct state *state = loop->state;
  close(state->kq);
  xalloc(state->events, 0);
  xalloc(state, 0);
}

static int api_realloc(struct loop *loop, int cap) {
  struct state *state = loop->state;
  state->events = xalloc(state->events, sizeof(struct kevent) * cap);
  if (unlikely(!state->events))
    goto err;
  state->revents = xalloc(state->revents, sizeof(unsigned char) * cap);
  if (unlikely(!state->revents))
    goto err;
  memset(state->revents, 0, sizeof(unsigned char) * cap);
  return 0;
err:
  if (state->events)
    xalloc(state->events, 0);
  if (state->revents)
    xalloc(state->revents, 0);
  return -1;
}

static int api_ctl(struct loop *loop, int op, int fd, int events) {
  struct state *state = loop->state;
  struct kevent ev;

  switch (op) {
  case EV_CTL_ADD:
  case EV_CTL_MOD:
    op = EV_ADD;
    break;
  case EV_CTL_DEL:
    op = EV_DELETE;
    break;
  default:
    return -2;
  }

  // This is way less cool than epoll :(

  if (events & EV_READ) {
    EV_SET(&ev, fd, EVFILT_READ, op, 0, 0, NULL);
    if (kevent(state->kq, &ev, 1, NULL, 0, NULL) < 0)
      return -2;
  }
  if (events & EV_WRITE) {
    EV_SET(&ev, fd, EVFILT_WRITE, op, 0, 0, NULL);
    if (kevent(state->kq, &ev, 1, NULL, 0, NULL) < 0)
      return -2;
  }
  return 0;
}

static int api_poll(struct loop *loop, struct timeval *tv) {
  struct state *state = loop->state;
  struct kevent *ev;
  struct timespec ts, *pts = NULL;
  int n, i, nevents = 0;

  if (tv) {
    ts.tv_sec = tv->tv_sec;
    ts.tv_nsec = tv->tv_usec * 1000;
    pts = &ts;
  }

  n = kevent(state->kq, NULL, 0, state->events, loop->cap, pts);
  if (unlikely(n < 0 && errno != EINTR)) {
    perror("api_poll:kqueue");
    exit(1);
  }

  unsigned char *pe;
  for (i = 0; i < n; i++) {
    ev = state->events + i;
    pe = state->revents + ev->ident;
    // merge fired events
    if (ev->filter == EVFILT_READ)
      *pe |= EV_READ;
    else if (ev->filter == EVFILT_WRITE)
      *pe |= EV_WRITE;
  }

  for (i = 0; i < n; i++) {
    ev = state->events + i;
    pe = state->revents + ev->ident;

    if (*pe) {
      loop->fired[i].fd = ev->ident;
      loop->fired[i].events = *pe;
      *pe = 0;  // reset
      nevents++;
    }
  }

  return nevents;
}

#endif
