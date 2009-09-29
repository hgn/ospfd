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


static int multiplex_ospf_packet(struct ospfd *ospfd, struct o_buf *o_buf)
{
	unsigned ospfd_packet_len;
	struct ipv4_ospf_header *hdr = o_buf->ospf_hdr.ipv4_ospf_header;

	ospfd_packet_len = o_buf->len - o_buf->inet_hdr_len;


	/* first of all: vality checks */
	if (sizeof(struct ipv4_ospf_header) > (size_t) ospfd_packet_len) {
		msg(ospfd, VERBOSE, "incoming packet is to small to contain a valid OSPF header"
				"is: %d must: %d (or bigger)",
				sizeof(struct ipv4_ospf_header), ospfd_packet_len);
		return FAILURE;
	}

	if (hdr->version != OSPF2_VERSION) {
		msg(ospfd, VERBOSE, "incoming packet is no OSPF version 2 packet");
		return FAILURE;
	}


	/* TODO: check header checksum and auth type */

	switch (hdr->type) {
		case MSG_TYPE_HELLO:
			/* set pointer of HELLO header */
			o_buf->data_hdr.ipv4_hello_header =
				(struct ipv4_hello_header *) o_buf->ospf_hdr.ipv4_ospf_header +
				sizeof(struct ipv4_ospf_header);
			return hello_ipv4_in(ospfd, o_buf);
			break;
		case MSG_TYPE_DATABASE_DESCRIPTION:
		case MSG_TYPE_LINK_STATE_REQUEST:
		case MSG_TYPE_LINK_STATE_UPDATE:
		case MSG_TYPE_LINK_STATE_ACK:
			msg(ospfd, VERBOSE, "OSPF packet type not handled!",
					hdr->type);
			break;
		default:
			msg(ospfd, VERBOSE, "incoming packet has no valid OSPF type: %d",
					hdr->type);
			return FAILURE;
	};

	return FAILURE;
}


static int multiplex_ipv4_packet(struct ospfd *ospfd, struct o_buf *o_buf)
{
	int ip_offset;
	const struct iphdr *iphdr = (struct iphdr *) o_buf->data;

	msg(ospfd, DEBUG, "handle incoming IPv4 packet");

	/* TODO: verify IPv4 header checksum and perform
	 * some IPv4 header sanity checks */

	/* find IP data offset - skip whole IPv4 header including
	 * all IP header options */
	ip_offset = iphdr->ihl * 4;
	o_buf->inet_hdr_len = ip_offset;

	if (iphdr->protocol == IPPROTO_OSPF) {
		o_buf->ospf_hdr.ipv4_ospf_header =
			(struct ipv4_ospf_header *) (o_buf->data + ip_offset);
		return multiplex_ospf_packet(ospfd, o_buf);
	}


	msg(ospfd, DEBUG, "incoming IPv4 packet not handled! Expected OSPF packet (%d) "
			"got packet of next header type of %d!\n", IPPROTO_OSPF, iphdr->protocol);

	return FAILURE;
}


static int multiplex_ipv6_packet(struct ospfd *ospfd, struct o_buf *o_buf)
{
	msg(ospfd, VERBOSE, "incoming IPv6 packet not handled, "
			"IPv6 support is still missing\n");

	(void) o_buf;

	return FAILURE;
}


static int multiplex_ip_packet(struct ospfd *ospfd, struct o_buf *o_buf)
{
	struct iphdr *iphdr;
	/* some dump sanity checks */

	/* additional OSPF header should be added here */
	int min_len = sizeof(struct iphdr) + sizeof(struct ipv4_ospf_header) +
		min(sizeof (struct ipv4_hello_header), sizeof(struct ipv4_hello_header));

	if (o_buf->len < min_len) {
		msg(ospfd, DEBUG, "incoming packet to small (is: %d must: %d)",
				o_buf->len, min_len);
		return FAILURE;
	}

	/* it does not matter if the ipv6 header or if the
	 * ipv4 header struct is used - the only required field
	 * is the ip version field and this field is identical in
	 * all protocol version   --HGN */
	iphdr = (struct iphdr *)o_buf->data;

	switch (iphdr->version) {
		case 4:
			return multiplex_ipv4_packet(ospfd, o_buf);
			break;
		case 6:
			return multiplex_ipv6_packet(ospfd, o_buf);
			break;
		default:
			msg(ospfd, DEBUG, "no valid IPv4 or IPv6 packet - "
					"IP version is: %d!", iphdr->version);
			return FAILURE;
			break;
	}

	return FAILURE;
}


static void save_msghdr_infos(struct o_buf *o_buf, struct msghdr *msghdr)
{
	struct cmsghdr *cmsgptr;

	for (cmsgptr = CMSG_FIRSTHDR(msghdr);
			cmsgptr != NULL;
			cmsgptr = CMSG_NXTHDR(msghdr, cmsgptr)) {

		if (cmsgptr->cmsg_len == 0)
			continue;

		if (cmsgptr->cmsg_level == SOL_IPV6 && cmsgptr->cmsg_type == IPV6_PKTINFO) {
			struct in6_pktinfo *pktinfo;
			pktinfo = (struct in6_pktinfo*)CMSG_DATA(cmsgptr);

			/* FIXME: the following line throws a compile time
			 * error for invalid derefernces of pktinfo - wtf? */
			/* o_buf->ifindex = pktinfo->ipi6_ifindex; */
		}

		if (cmsgptr->cmsg_level == SOL_IP && cmsgptr->cmsg_type == IP_PKTINFO) {
			struct in_pktinfo *pktinfo;
			pktinfo = (struct in_pktinfo*)CMSG_DATA(cmsgptr);

			o_buf->ifindex = pktinfo->ipi_ifindex;
		}
	}
}


/* called from event mechanism if socket
 * status changes - normally to readable state */
void packet_input(int fd, int what, void *priv_data)
{
	int ret;
	struct ospfd *ospfd;
	struct o_buf o_buf;
	char raw_packet_buf[MAX_PACKET_SIZE];

	(void) what;

	ospfd = priv_data;

	init_o_buf(&o_buf);

	o_buf.data = raw_packet_buf;

	struct iovec iov;
	struct msghdr msghdr;
	char ancillary[64];
	union {
		struct sockaddr_in in4;
		struct sockaddr_in6 in6;
	} addr_src;

	msghdr.msg_name = &addr_src;
	msghdr.msg_namelen = sizeof(addr_src);
	iov.iov_base = raw_packet_buf;
	iov.iov_len = sizeof(raw_packet_buf);

	msghdr.msg_iov = &iov;
	msghdr.msg_iovlen = 1;
	msghdr.msg_control = ancillary;
	msghdr.msg_controllen = sizeof(ancillary);
	msghdr.msg_flags = 0;

	while ((o_buf.len = recvmsg(fd, &msghdr, 0)) > 0) {

		save_msghdr_infos(&o_buf, &msghdr);

		ospfd->network.stats.packets_rx++;
		/* try to guess if raw_packet_buf is ipv4 or ipv6 and has a valid
		 * OSPF header */
		ret = multiplex_ip_packet(ospfd, &o_buf);
		if (ret != SUCCESS) {
			msg(ospfd, DEBUG, "cannot multiplex incoming packet");
			return;
		}
	}


	if (ret < 0 && errno != EWOULDBLOCK) {
		err_sys("failure in read(2) operation for raw socket");
		return;
	}
}


/* vim: set tw=78 ts=4 sw=4 sts=4 ff=unix noet: */
