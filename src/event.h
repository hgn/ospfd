#ifndef EVENT_H
#define	EVENT_H

#include "ospfd.h"
#include <inttypes.h>
#include <sys/epoll.h>

#define	EVENT_USER_SOVEREIGNTY (1 << 0)
#define	EVENT_ONE_SHOT         (1 << 1)
#define	EVENT_REPEAT           (1 << 2)

int ev_add(struct ospfd *ospfd, int, void (*cb)(int, void *), void *, uint32_t, int32_t);
int ev_add_sovereignty(struct ospfd *, int ,void (*cb)(int, void *), void *, uint32_t, struct ev_data **);
int ev_loop(struct ospfd *);
int ev_init(struct ospfd *);
int ev_del(struct ospfd *, int);

#endif /* EVENT_H */

/* vim:set ts=4 sw=4 sts=4 tw=78 ff=unix noet: */
