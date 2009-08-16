#ifndef RC_H
#define	RC_H

#include "ospfd.h"

int parse_rc_file(struct ospfd *);
void rc_add_area(char *);
void rc_add_id(char *);

#endif /* RC_H */

/* vim:set ts=4 sw=4 sts=4 tw=78 ff=unix noet: */
