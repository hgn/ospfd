#ifndef NETWORK_H
#define	NETWORK_H

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>

#include "ospfd.h"
#include "ospf_packets.h"

#ifndef IPPROTO_OSPF
#define IPPROTO_OSPF 89
#endif

#define DEFAULT_OSPF_TTL 1

/* see http://en.wikipedia.org/wiki/Open_Shortest_Path_First :-) */
#define MCAST_ALL_SPF_ROUTERS "224.0.0.5"
#define MCAST_ALL_DROUTERS    "224.0.0.6"


#define	O_BUF_INF_UNKOWN ((unsigned int)-1)

/* similar to {sk,m}_buf - a packet wrapper
 * that includes all required information for
 * the packet journey through the OSPF stack */
struct o_buf {
	char *data;
	ssize_t len;

	union {
		struct iphdr *iphdr;
	} inet_hdr;
	ssize_t inet_hdr_len;

	union {
		struct ipv4_ospf_header *ipv4_ospf_header;
	} ospf_hdr;

	union {
		struct ipv4_hello_header *ipv4_hello_header;
	} data_hdr;

	unsigned int ifindex;
};

int init_network(struct ospfd *);
void fini_network(struct ospfd *);
uint16_t calc_fl_checksum(char *, uint16_t, uint16_t);
void init_o_buf(struct o_buf *);
int list_cmp_struct_ip_addr(const void *, const void *);
void free_ip_addr(void *);
int list_cmp_rd(const void *, const void *);
void list_free_rd(void *);

/* rx.c */
int ipv4_input(const char *);
void packet_input(int, int, void *);


#endif /* NETWORK_H */

/* vim:set ts=4 sw=4 sts=4 tw=78 ff=unix noet: */
