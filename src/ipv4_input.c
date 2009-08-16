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

	ret = read(fd, packet, MAX_PACKET_SIZE);
	if (ret < 0) {
		err_msg("failure in read(2) operation for raw socket");
		return;
	}

	/* try to guess if packet is ipv4 or ipv6 and has a valid
	 * OSPF header */
	fprintf(stderr, "new incoming packet\n");

}



/* vim: set tw=78 ts=4 sw=4 sts=4 ff=unix noet: */
