#ifndef OSPF_PHT_H
#define OSPF_PHT_H

#include <inttypes.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ospfd.h"

#define OSPF_HEADER_SIZE 24u
#define OSPF_AUTH_SIMPLE_SIZE 8u
#define OSPF_AUTH_MD5_SIZE 16u

#define OSPF_MAX_PACKET_SIZE 65535u
#define OSPF_HELLO_MIN_SIZE 20u

#define	OSPF2_VERSION 2

/* 4.3. Routing protocol packets */
#define	MSG_TYPE_HELLO                1 /* Discover and maintain  neighbors */
#define	MSG_TYPE_DATABASE_DESCRIPTION 2 /* Summarize database contents */
#define	MSG_TYPE_LINK_STATE_REQUEST   3 /* Database download */
#define	MSG_TYPE_LINK_STATE_UPDATE    4 /* Database update */
#define	MSG_TYPE_LINK_STATE_ACK       5 /* Flooding acknowledgment */

#define	AUTH_TYPE_NULL 0

/* 8.1.  Sending protocol packets */
struct hello_ipv4_std_header {

	/* Version -- set to 2, the version number of the protocol as
	   documented in this specification. */
	uint8_t version;

	/* Type -- The type of OSPF packet, such as Link state Update
       or Hello packet */
	uint8_t type;

	/* Packet length -- The length of the entire OSPF packet in
       bytes, including the standard OSPF packet header. */
	uint16_t length;

	/* Router ID -- The identity of the router itself (who is
       originating the packet). */
	uint32_t router_id;

	/* Area ID -- The OSPF area that the packet is being sent into. */
	uint32_t area_id;

	/* Checksum -- The standard IP 16-bit one's complement
	   checksum of the entire OSPF packet, excluding the 64-bit
       authentication field. This checksum is calculated as part
       of the appropriate authentication procedure; for some OSPF
       authentication types, the checksum calculation is omitted.
       See Section D.4 for details. */
	uint16_t checksum;

	/* AuType and Authentication -- Each OSPF packet exchange is
       authenticated.  Authentication types are assigned by the
       protocol and are documented in Appendix D. A different
       authentication procedure can be used for each IP network/subnet.
       Autype indicates the type of authentication procedure in use.
       The 64-bit authentication field is then for use by the chosen
       authentication procedure.  This procedure should be the last
       called when forming the packet to be sent. See Section D.4 for details. */
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

/* Section  9.5 */
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
