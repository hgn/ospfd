#ifndef HELLO_H
#define HELLO_H

#include <inttypes.h>

#include "ospfd.h"
#include "ospf_pkts.h"

void tx_ipv4_hello_packet(int, void *);

struct tx_hello_arg {
	struct ospfd *ospfd;
	/* rc_rd is a pointer to the actual
	 * domain that should be processed by this
	 * particular callback */
	struct rc_rd *rc_rd;
};


#endif /* TIMER_H */

/* vim: set tw=78 ts=4 sw=4 sts=4 ff=unix noet: */
