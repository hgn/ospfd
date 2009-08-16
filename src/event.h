#ifndef EVENT_H
#define	EVENT_H

#include "ospfd.h"
#include <inttypes.h>
#include <sys/epoll.h>


int ev_add(struct ospfd *ospfd, int, void (*cb)(int, void *), void *, uint32_t);
int ev_loop(struct ospfd *);
int ev_init(struct ospfd *);

#endif /* EVENT_H */

/* vim:set ts=4 sw=4 sts=4 tw=78 ff=unix noet: */
