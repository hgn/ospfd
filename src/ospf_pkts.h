#ifndef OSPF_PHT_H
#define OSPF_PHT_H

#include <inttypes.h>

#include "ospfd.h"

#define OSPF_HEADER_SIZE 24u
#define OSPF_AUTH_SIMPLE_SIZE 8u
#define OSPF_AUTH_MD5_SIZE 16u

#define OSPF_MAX_PACKET_SIZE 65535u
#define OSPF_HELLO_MIN_SIZE 20u

#define	OSPF2_VERSION 2

#define	MESSAGE_TYPE_HELLO 1

#define	AUTH_TYPE_NULL 0

struct hello_ipv4_std_header {
	uint8_t version;
	uint8_t type;
	uint16_t length;
	uint32_t router_id;
	uint32_t area_id;
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




#endif /* OSPF_PHT_H */

/* vim: set tw=78 ts=4 sw=4 sts=4 ff=unix noet: */
