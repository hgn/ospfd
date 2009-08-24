#ifndef NETWORK_H
#define	NETWORK_H

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>

#include "ospfd.h"
#include "ospf_pkts.h"

#ifndef IPPROTO_OSPF
#define IPPROTO_OSPF 89
#endif

#define DEFAULT_OSPF_TTL 1

/* see http://en.wikipedia.org/wiki/Open_Shortest_Path_First :-) */
#define MCAST_ALL_SPF_ROUTERS "224.0.0.5"
#define MCAST_ALL_DROUTERS    "224.0.0.6"


int init_network(struct ospfd *);
void fini_network(struct ospfd *);

/* ipv4_input.c */
int ipv4_input(const char *);
void packet_input(int, void *);


#endif /* NETWORK_H */

/* vim:set ts=4 sw=4 sts=4 tw=78 ff=unix noet: */
