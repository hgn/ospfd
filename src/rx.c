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

#define	MAX_PACKET_SIZE 4096

int ipv4_input(const char *packet)
{
	int ret = SUCCESS;

	return ret;
}

static int multiplex_ip_packet(struct ospfd *ospfd,
		char *packet, ssize_t packet_len)
{
	/* some dump sanity checks */

	/* additional OSPF header should be added here */
	int min_len = sizeof(struct iphdr) + sizeof(struct hello_ipv4_std_header) +
		min(sizeof (struct ipv4_hello_header), sizeof(struct ipv4_hello_header));

	if (packet_len < min_len) {
		msg(ospfd, DEBUG, "incoming packet to small (is: %d must: %d)",
				packet_len, min_len);
		return FAILURE;
	}

	return SUCCESS;
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
			msg(ospfd, DEBUG, "cannot mutliplex packet");
			return;
		}
	}
	if (ret < 0 && errno != EWOULDBLOCK) {
		err_sys("failure in read(2) operation for raw socket");
		return;
	}
}


/* vim: set tw=78 ts=4 sw=4 sts=4 ff=unix noet: */
