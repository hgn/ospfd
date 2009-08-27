#ifndef NBR_H
#define NBR_H

#include "ospfd.h"

void nbr_set_state(struct ospfd *ospfd, struct rc_rd *rc_rd, int new_state);
struct rc_rd *alloc_rc_rd(void);

#endif /* NBR_H */
