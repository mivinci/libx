#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "x/ev.h"
#include "x/x.h"

struct ev_fired {
  int fd;
  int events;  // fired events (like event from poll.h)
};

struct loop {
  int maxfd;               // maximum file discriptor of IO events.
  int cap;                 // the number of slots allocated for loop::events;
  int len;                 // the number of timer events.
  int len_io;              // the number of IO events.
  struct ev_fired *fired;  // events fired.
  struct ev **events;      // events being watched, indexed by ev::fd.
  struct ev **heap;        // minheap for timer events.
  void *state;             // implementation-specific data.
};

#ifdef __linux__
#include "ev_epoll.c"
#else
#ifndef __APPLE__
#include "ev_kqueue.c"
#else
#include "ev_select.c"
#endif
#endif

struct loop *loop_alloc(int backlog) {
  struct loop *loop;

  loop = xalloc(NULL, sizeof(struct loop));
  if (!loop)
    goto err;
  memset(loop, 0, sizeof(struct loop));

  loop->fired = xalloc(NULL, sizeof(struct ev_fired) * backlog);
  if (!loop->fired)
    goto err;

  loop->events = xalloc(NULL, sizeof(struct ev *) * backlog);
  if (!loop->events)
    goto err;

  loop->heap = xalloc(NULL, sizeof(struct ev *) * backlog);
  if (!loop->heap)
    goto err;

  loop->cap = backlog;
  loop->len = 0;

  if (api_init(loop) < 0)
    goto err;

  return loop;

err:
  if (loop && loop->heap)
    xalloc(loop->heap, 0);
  if (loop && loop->events)
    xalloc(loop->events, 0);
  if (loop && loop->fired)
    xalloc(loop->fired, 0);
  if (loop)
    xalloc(loop, 0);
  return NULL;
}

// less returns 1(0) if 'p' is sooner(later) than 'q'.
static inline int less(struct ev *p, struct ev *q) {
  struct timeval *tp = &p->exp, *tq = &q->exp;
  return tp->tv_sec < tq->tv_sec ||
         (tp->tv_sec == tq->tv_sec && tp->tv_usec < tq->tv_usec);
}

// heap_up moves heap[i] upwards.
static inline void heap_up(struct ev **heap, int i) {
  struct ev *tmp;
  int j;
  for (;;) {
    j = (i - 1) / 2;  // parent
    if (j == i || less(heap[j], heap[i]))
      break;
    tmp = heap[i];
    heap[i] = heap[j];
    heap[j] = tmp;
    i = j;
  }
}

// heap_up moves heap[i] downwards and returns 1(0) if it is(not) moved.
static inline int heap_down(struct ev **heap, int i, int n) {
  struct ev *tmp;
  int t = i, j, j1, j2;
  for (;;) {
    j1 = 2 * i + 1;  // left child
    if (j1 >= n)
      break;
    j = j1;

    j2 = j1 + 1;  // right child
    if (j2 < n && less(heap[j2], heap[j1]))
      j = j2;

    if (less(heap[i], heap[j]))
      break;
    tmp = heap[i];
    heap[i] = heap[j];
    heap[j] = tmp;
    i = j;
  }
  return i > t;
}

// heap_remove removes heap[i] and re-order the heap.
static struct ev *heap_remove(struct ev **heap, int i, int n) {
  struct ev *ev = heap[i];
  if (i != n) {  // if we are not removing the last one.
    heap[i] = heap[n - 1];
    if (!heap_down(heap, i, n))
      heap_up(heap, i);
  }
  return ev;
}

// heap_push pushes 'ev' up to the correct position in the heap.
static void heap_push(struct ev **heap, struct ev *ev, int n) {
  heap[n] = ev;
  heap_up(heap, n);
}

// heap_pop pops out heap[0] and re-order the heap.
static struct ev *heap_pop(struct ev **heap, int n) {
  struct ev *ev;
  if (n <= 0)
    return NULL;
  ev = heap[0];
  heap[0] = heap[n - 1];
  heap_down(heap, 0, n);
  return ev;
}

int loop_dispatch(struct loop *loop, int flags) {
  struct timeval now, tv, *ptv = NULL;
  struct ev *tev = NULL;
  struct ev *event = NULL;
  struct ev_fired *fired = NULL;
  int i, err, nevents, polled = 0;

  // a zero flag means the caller doesn't want to dispatch
  // any event, so we return right away.
  if (!flags)
    return 0;

  // see if or not the caller wants to dispatch a timer event.
  // if not, we go for IO events.
  if (!(flags & EV_TIMER))
    goto do_io;

  // get the current time in microseconds.
  if (unlikely(gettimeofday(&now, NULL) < 0))
    return -1;

  // poll the closest timer event, if there's one then we use
  // its timeout interval to timeout the polling of IO events
  if ((tev = heap_pop(loop->heap, loop->len)) != NULL) {
    tv.tv_sec = tev->exp.tv_sec - now.tv_sec;
    tv.tv_usec = tev->exp.tv_usec - now.tv_usec;
    ptv = &tv;
    loop->len--;
    tev->id = -1;  // avoid duplicated removal by loop_ctl
  }

  // if the closest timer event we just polled out reaches or
  // exceeds its timeout interval, we must go dispatch it
  // immediately, leaving the IO events to the next call to
  // loop_dispatch.
  // PS: if "tv_usec" is 900, adding 999 and then dividing by
  // 1000 will give a result of 1, which is the nearest millisecond.
  // if we simply divide by 1000 without adding 999, the result
  // would be 0, which is incorrect.
  if (ptv && ptv->tv_sec * 1000 + (ptv->tv_usec + 999) / 1000 <= 0)
    goto do_timer;

do_io:
  // if the caller doesn't want to dispatch IO events either,
  // we return right away.
  if (!(flags & EV_READ) && !(flags & EV_WRITE))
    return 0;

  // poll fired IO events with the timeout interval of the closest
  // timer event we just calculated.
  nevents = api_poll(loop, ptv);

  // dispatch fired IO events to their callback functions.
  for (i = 0; i < nevents; i++) {
    fired = &loop->fired[i];
    event = loop->events[fired->fd];
    event->revents = fired->events;
    if (event->callback) {
      if ((err = event->callback(loop, event)) < 0)
        return err;
      polled++;
    }
  }

do_timer:
  if (tev && tev->callback) {
    tev->revents = EV_TIMER;
    if ((err = tev->callback(loop, tev)) < 0)
      return err;
    if (tev->fd > 0)  // remove the event if it is also an IO event.
      api_ctl(loop, EV_CTL_DEL, tev->fd, tev->events);
    polled++;
  }
  return polled;
}

int loop_wait(struct loop *loop) {
  int polled = 0, n;
  while (loop->len + loop->len_io) {
    if ((n = loop_dispatch(loop, EV_ALL)) < 0)
      return n;
    polled += n;
  }
  return polled;
}

static inline int __realloc(struct loop *loop, int cap) {
  struct ev_fired *fired;
  struct ev **events, **heap;

  fired = xalloc(loop->fired, sizeof(*fired) * cap);
  if (unlikely(!fired))
    return -1;

  events = xalloc(loop->events, sizeof(*events) * cap);
  if (unlikely(!events))
    return -1;

  heap = xalloc(loop->heap, sizeof(*heap) * cap);
  if (unlikely(!heap))
    return -1;

  loop->fired = fired;
  loop->events = events;
  loop->heap = heap;
  return 0;
}

int loop_ctl(struct loop *loop, int op, struct ev *ev) {
  struct timeval now;
  int status, cap;

  switch (op) {
  case EV_CTL_ADD:
    // extend the event loop if needed.
    if (loop->maxfd >= loop->cap) {
      cap = loop->cap + loop->cap / 2;  // 1.5x the current capacity
      status = __realloc(loop, cap);
      if (unlikely(status < 0))
        return status;
      status = api_realloc(loop, cap);
      if (unlikely(status < 0))
        return status;
      loop->cap = cap;
    }
    // add ev to the kernel if it is an IO event.
    if (ev->events & EV_IO) {
      status = api_ctl(loop, EV_CTL_ADD, ev->fd, ev->events);
      if (unlikely(status < 0))
        return status;
      loop->len_io++;
    }
    if (loop->maxfd < ev->fd)
      loop->maxfd = ev->fd;
    // add ev to the minimal heap if it is a timeout event.
    if (ev->events & EV_TIMER) {
      if (ev->ms) {
        status = gettimeofday(&now, NULL);
        if (unlikely(status < 0))
          return status;
        ev->exp.tv_sec = now.tv_sec + ev->ms / 1000;
        ev->exp.tv_usec = now.tv_usec + ev->ms % 1000 * 1000;
      }
      heap_push(loop->heap, ev, loop->len);
      loop->len++;
    }
    // save ev in the event loop.
    loop->events[ev->fd] = ev;
    return 0;  // we are good to go :)

  case EV_CTL_DEL:
    // remove ev from the kernel if it is an IO event.
    if (ev->events & EV_IO)
      status = api_ctl(loop, op, ev->fd, ev->events);
    // remove ev from the minimal heap if it is a timeout event.
    if ((ev->events & EV_TIMER) && ev->id >= 0) {
      heap_remove(loop->heap, ev->id, loop->len);
      loop->len--;
    }
    // remove ev from the event loop anyways.
    if (op == EV_CTL_DEL) {
      loop->events[ev->fd] = NULL;
      loop->len_io--;
    }
    return 0;

  default:
    return -2;  // :(
  }
}

void loop_free(struct loop *loop) {
  api_free(loop);
  xalloc(loop->fired, 0);
  xalloc(loop->events, 0);
  xalloc(loop->heap, 0);
  xalloc(loop, 0);
}
