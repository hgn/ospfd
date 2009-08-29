#ifndef RC_H
#define	RC_H

#include "ospfd.h"

int parse_rc_file(struct ospfd *);
void rc_set_area(char *, char *);
void rc_set_costs(char *, char *);
void rc_show_interface(char *);
void rc_set_ipv4_address(char *, char *, char *);
void rc_set_description(char *, char *);
void rc_set_hello_interval(char *, char *);
void rc_set_id(char *);
void rc_set_interface_up(char *);
void rc_set_router_priority(char *, char *);
void rc_set_router_dead_interval(char *, char *);

#endif /* RC_H */

/* vim:set ts=4 sw=4 sts=4 tw=78 ff=unix noet: */
