#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>


#include "network.h"
#include "shared.h"

#define	MAX_PACKET_SIZE 4096

int ipv4_input(const char *packet)
{
	int ret = SUCCESS;

	return ret;
}

/* called from event mechanism if socket
 * status changes - normally to readable state */
void packet_input(int fd, void *priv_data)
{
	int ret;
	char *packet[MAX_PACKET_SIZE];

	(void) priv_data;

	while ((ret = read(fd, packet, MAX_PACKET_SIZE)) > 0) {
		/* try to guess if packet is ipv4 or ipv6 and has a valid
		 * OSPF header */
		fprintf(stderr, "new incoming packet of size %d\n", ret);
	}
	if (ret < 0 && errno != EWOULDBLOCK) {
		err_sys("failure in read(2) operation for raw socket");
		return;
	}
}



/* vim: set tw=78 ts=4 sw=4 sts=4 ff=unix noet: */
