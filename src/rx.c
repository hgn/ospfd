#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>


#include "ospfd.h"
#include "network.h"
#include "shared.h"
#include "hello_rx.h"

#define	MAX_PACKET_SIZE 4096

static int multiplex_ospf_packet(struct ospfd *ospfd,
		char *packet, ssize_t packet_len)
{
	const struct hello_ipv4_std_header *hdr;


	hdr = (struct hello_ipv4_std_header *)packet;

	/* first of all: vality checks */
	if (sizeof(struct hello_ipv4_std_header) > (size_t) packet_len) {
		msg(ospfd, VERBOSE, "incoming packet is to small to contain a valid OSPF header"
				"is: %d must: %d (or bigger)",
				sizeof(struct hello_ipv4_std_header), packet_len);
		return FAILURE;
	}

	if (hdr->version != OSPF2_VERSION) {
		msg(ospfd, VERBOSE, "incoming packet is no OSPF version 2 packet");
		return FAILURE;
	}


	/* TODO: check header checksum and auth type */

	switch (hdr->type) {
		case MSG_TYPE_HELLO:
			return hello_ipv4_in(ospfd, packet, packet_len);
			break;
		case MSG_TYPE_DATABASE_DESCRIPTION:
			break;
		case MSG_TYPE_LINK_STATE_REQUEST:
			break;
		case MSG_TYPE_LINK_STATE_UPDATE:
			break;
		case MSG_TYPE_LINK_STATE_ACK:
			break;
		default:
			msg(ospfd, VERBOSE, "incoming packet has no valid OSPF type: %d",
					hdr->type);
			return FAILURE;
	};

	return FAILURE;
}

static int multiplex_ipv4_packet(struct ospfd *ospfd,
		char *packet, ssize_t packet_len)
{
	int ip_offset;
	const struct iphdr *iphdr = (struct iphdr *) packet;

	msg(ospfd, DEBUG, "handle incoming IPv4 packet");

	/* TODO: verify IPv4 header checksum and perform
	 * some IPv4 header sanity checks */

	/* find IP data offset - skip whole IPv4 header including
	 * all IP header options */
	ip_offset = iphdr->ihl * 4;

	if (iphdr->protocol == IPPROTO_OSPF)
		return multiplex_ospf_packet(ospfd, packet + ip_offset, packet_len - ip_offset);


	msg(ospfd, DEBUG, "incoming IPv4 packet not handled! Expected OSPF packet (%d) "
			"got packet of next header type of %d!\n", IPPROTO_OSPF, iphdr->protocol);

	return FAILURE;
}

static int multiplex_ipv6_packet(struct ospfd *ospfd,
		char *packet, ssize_t packet_len)
{
	msg(ospfd, VERBOSE, "incoming IPv6 packet not handled, "
			"IPv6 support is still missing\n");

	(void) packet;
	(void) packet_len;

	return FAILURE;
}

static int multiplex_ip_packet(struct ospfd *ospfd,
		char *packet, ssize_t packet_len)
{
	struct iphdr *iphdr;
	/* some dump sanity checks */

	/* additional OSPF header should be added here */
	int min_len = sizeof(struct iphdr) + sizeof(struct hello_ipv4_std_header) +
		min(sizeof (struct ipv4_hello_header), sizeof(struct ipv4_hello_header));

	if (packet_len < min_len) {
		msg(ospfd, DEBUG, "incoming packet to small (is: %d must: %d)",
				packet_len, min_len);
		return FAILURE;
	}

	/* it does not matter if the ipv6 header or if the
	 * ipv4 header struct is used - the only required field
	 * is the ip version field and this field is identical in
	 * all protocol version   --HGN */
	iphdr = (struct iphdr *)packet;

	switch (iphdr->version) {
		case 4:
			return multiplex_ipv4_packet(ospfd, packet, packet_len);
			break;
		case 6:
			return multiplex_ipv6_packet(ospfd, packet, packet_len);
			break;
		default:
			msg(ospfd, DEBUG, "no valid IPv4 or IPv6 packet - "
					"IP version is: %d!", iphdr->version);
			return FAILURE;
			break;
	}

	return FAILURE;
}

/* called from event mechanism if socket
 * status changes - normally to readable state */
void packet_input(int fd, void *priv_data)
{
	ssize_t pret; int ret;
	struct ospfd *ospfd;
	char packet[MAX_PACKET_SIZE];

	ospfd = priv_data;

	while ((pret = read(fd, packet, MAX_PACKET_SIZE)) > 0) {

		ospfd->network.stats.packets_rx++;
		/* try to guess if packet is ipv4 or ipv6 and has a valid
		 * OSPF header */
		ret = multiplex_ip_packet(ospfd, packet, pret);
		if (ret != SUCCESS) {
			msg(ospfd, DEBUG, "cannot multiplex packet");
			return;
		}
	}
	if (ret < 0 && errno != EWOULDBLOCK) {
		err_sys("failure in read(2) operation for raw socket");
		return;
	}
}


/* vim: set tw=78 ts=4 sw=4 sts=4 ff=unix noet: */
