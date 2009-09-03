#ifndef HELLO_RX_H
#define HELLO_RX_H

#include <inttypes.h>

#include "ospfd.h"
#include "network.h"
#include "ospf_packets.h"

int hello_ipv4_in(struct ospfd *, struct o_buf *);


#endif /* HELLO_RX_H */

/* vim: set tw=78 ts=4 sw=4 sts=4 ff=unix noet: */
