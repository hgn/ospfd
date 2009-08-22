#ifndef HELLO_H
#define HELLO_H

#include <inttypes.h>

#include "ospfd.h"

#define OSPF_HEADER_SIZE 24u
#define OSPF_AUTH_SIMPLE_SIZE 8u
#define OSPF_AUTH_MD5_SIZE 16u

#define OSPF_MAX_PACKET_SIZE 65535u
#define OSPF_HELLO_MIN_SIZE 20u

struct ipv4_std_header {
	uint8_t version;
	uint8_t type;
	uint16_t length;
	struct in_addr router_id;
	struct in_addr area_id;
	uint16_t checksum;
	uint16_t auth_type;
	union
	{
		uint8_t auth_data[OSPF_AUTH_SIMPLE_SIZE];
		struct
		{
			uint16_t unused;
			uint8_t key_id;
			uint8_t auth_data_len;
			uint32_t crypt_seq_number;
		} crypt;
	} u;
};

struct ipv4_hello_header {
	struct in_addr network_mask;
	uint16_t hello_interval;
	uint8_t options;
	uint8_t priority;
	uint32_t dead_interval;
	struct in_addr d_router;
	struct in_addr bd_router;
	struct in_addr neighbors[1];
};

struct db_description
{
	uint16_t mtu;
	uint8_t options;
	uint8_t flags;
	uint32_t dd_seqnum;
};


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
