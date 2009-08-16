#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "network.h"
#include "shared.h"
#include "event.h"

static int join_router_4_multicast(int fd)
{
	int ret = SUCCESS;
	struct ip_mreqn mreqn;

	/* FIXME: we does not set any of imr_address or
	 * imr_ifindex because 0 works fine for us now.
	 * This should be fixed with a proper routine */
	memset(&mreqn, 0, sizeof(struct ip_mreqn));

	ret = inet_aton(MCAST_ALL_SPF_ROUTERS, &mreqn.imr_multiaddr);
	if (ret < 0) {
		err_sys("failed to convert router multicast address");
		return FAILURE;
	}

	ret = setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreqn, sizeof(mreqn));
	if (ret < 0) {
		err_sys("failed to add multicast membership");
		return FAILURE;
	}


	return ret;
}

static int init_raw_4_socket(struct ospfd *ospfd)
{
	int fd, on = 1, ret;

	fd = socket(AF_INET, SOCK_RAW, IPPROTO_OSPF);
	if (fd < 0) {
		err_sys("Failed to create raw IPPROTO_OSPF socket");
		return FAILURE;
	}

	ret = setsockopt(fd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on));
	if (ret < 0) {
		err_sys("Failed to set IP_HDRINCL socket option");
		return FAILURE;
	}

	ret = setsockopt(fd, IPPROTO_IP, IP_PKTINFO, &on, sizeof(on));
	if (ret < 0) {
		err_sys("Failed to set IP_PKTINFO socket option");
		return FAILURE;
	}

	ret = join_router_4_multicast(fd);
	if (ret != SUCCESS) {
		err_msg("cannot join multicast groups");
		return FAILURE;
	}

	ospfd->network.fd = fd;

	return SUCCESS;
}

static void fini_raw_socket(struct ospfd *ospfd)
{
	close(ospfd->network.fd);
}


int init_network(struct ospfd *ospfd)
{
	int ret;
	uint32_t flags;

	switch (ospfd->opts.family) {
		case AF_INET:
			ret = init_raw_4_socket(ospfd);
			if (ret != SUCCESS) {
				err_msg("Failed to initialize raw socket");
				return FAILURE;
			}
			/* and register fd */
			flags = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP;
			ret = ev_add(ospfd, ospfd->network.fd, packet_input, NULL, flags);
			if (ret < 0) {
				err_msg("cannot at RAW socket to event mechanism");
				return FAILURE;
			}
			break;
		default:
			err_msg("Protocol family not supported (%d)", ospfd->opts.family);
			return FAILURE;
	}

	return SUCCESS;
}

void fini_network(struct ospfd *ospfd)
{
	switch (ospfd->opts.family) {
		case AF_INET:
			fini_raw_socket(ospfd);
			break;
		default:
			err_msg("Protocol family not supported (%d)", ospfd->opts.family);
			return;
	}
}



/* vim: set tw=78 ts=4 sw=4 sts=4 ff=unix noet: */
