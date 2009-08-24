#ifndef EVENT_H
#define	EVENT_H

#include "ospfd.h"
#include <inttypes.h>
#include <sys/epoll.h>

#define	EVENT_ONE_SHOT (1 << 0)
#define	EVENT_REPEAT   (1 << 1)

/* WARNING: this event mechanism is decide for two
 * use cases: handle fd with unvinite lifetime or handle
 * filedescriptors with a one-shot lifetime that is removed
 * after the first call. If not than this event handling system
 * must adjusted to let hand the sovereignty over to the calle
 * of the internal event handling/multiplexing structure. See
 * event.c and in particular struct ev_data. The proposed
 * solution is sufficient now - but it does not neccessary
 * mean /infty{} --HGN */

int ev_add(struct ospfd *ospfd, int, void (*cb)(int, void *), void *, uint32_t, int32_t);
int ev_loop(struct ospfd *);
int ev_init(struct ospfd *);
int ev_del(struct ospfd *, int);

#endif /* EVENT_H */

/* vim:set ts=4 sw=4 sts=4 tw=78 ff=unix noet: */
