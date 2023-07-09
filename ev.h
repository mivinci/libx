#ifndef _X_EV_H
#define _X_EV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/time.h>

#define EV_CTL_ADD 1
#define EV_CTL_DEL 2
#define EV_CTL_MOD 3

#define EV_NULL  0
#define EV_READ  (1 << 1)
#define EV_WRITE (1 << 2)
#define EV_TIMER (1 << 3)
#define EV_IO    (EV_READ | EV_WRITE)
#define EV_ALL   (EV_READ | EV_WRITE | EV_TIMER)

// struct loop represents an event loop.
struct loop;

// struct ev represents an IO event or a timer event.
struct ev {
  /* public  */

  // file descriptor for an IO event.
  int fd;
  // events to be watched, can be EV_READ, EV_WRITE for
  // an IO event, or EV_TIMER for an timer event, or both.
  int events;
  // milliseconds to wait for a timer event.
  long long ms;
  // callback function for an event.
  void (*callback)(struct loop *, unsigned, struct ev *);

  /* private */

  // absolute time to fire an timer event, initialized by
  // the event loop.
  struct timeval exp;
  // index into the minheap for an timer event, initialized
  // by the event loop.
  int id;
};

/* Event Loop Primitives */

// loop_new creates an event loop.
struct loop *loop_new(int);
// loop_dispatch pools and dispatches fired events to their
// callback functions,
int loop_dispatch(struct loop *, int);
// loop_sched calls loop_dispatch in an infinite loop on all
// events, returns the toal amount of dispatched events if the
// event loop exits.
int loop_sched(struct loop *);
// loop_ctl adds, modifies or deletes an event in the event
// loop, returns 0 on success or a negative number on an error
int loop_ctl(struct loop *, int, struct ev *);
// loop_free frees memories allocated by the event loop, and
// removes all event being watched from the kernel.
void loop_free(struct loop *);

/* Useful Macros */

#define loop_add(L, e) loop_ctl(L, EV_CTL_ADD, e)
#define loop_del(L, e) loop_ctl(L, EV_CTL_DEL, e)
#define loop_mod(L, e) loop_ctl(L, EV_CTL_MOD, e)

#ifdef __cplusplus
}
#endif

#endif
