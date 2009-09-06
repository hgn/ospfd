#ifndef TIMER_H
#define TIMER_H

#include "ospfd.h"
#include "rbtree.h"

int timer_add_ns_rel(struct ospfd *, time_t, long, void (*cb)(int, void *), void *);
int timer_add_s_rel(struct ospfd *, unsigned int, void (*cb)(int, void *), void *);
int timer_del(struct ospfd *, int);
int timer_del_early(struct ospfd *, int);
int timer_add_ns_rel_sovereignty(struct ospfd *, time_t, long, void (*cb)(int, void *), void *, struct ev_data **, int *);
int timer_add_s_rel_sovereignty(struct ospfd *, unsigned int, void (*cb)(int, void *), void *, struct ev_data **, int *);


#endif /* TIMER_H */

/* vim: set tw=78 ts=4 sw=4 sts=4 ff=unix noet: */
