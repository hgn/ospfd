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

static void print_all_addresses(const void *data)
{
	const struct ip_addr *ip_addr = (struct ip_addr *) data;
	char addr[INET6_ADDRSTRLEN], mask[INET6_ADDRSTRLEN], broadcast[INET_ADDRSTRLEN];

	switch (ip_addr->family) {
		case AF_INET:
		case AF_PACKET:
			inet_ntop(AF_INET, &ip_addr->ipv4.addr, addr, INET6_ADDRSTRLEN);
			inet_ntop(AF_INET, &ip_addr->ipv4.netmask, mask, INET6_ADDRSTRLEN);
			inet_ntop(AF_INET, &ip_addr->ipv4.broadcast, broadcast, INET6_ADDRSTRLEN);
			fprintf(stdout, "  inet:  %s netmask: %s broadcast: %s\n", addr, mask, broadcast);
			break;
		case AF_INET6:
			inet_ntop(AF_INET6, &ip_addr->ipv6.addr, addr, INET6_ADDRSTRLEN);
			inet_ntop(AF_INET6, &ip_addr->ipv6.netmask, mask, INET6_ADDRSTRLEN);
			fprintf(stdout, "  inet6: %s netmask: %s\n", addr, mask);
			break;
		default:
			/* should not happened - catched several times earlier */
			break;
	}
	return;
}

static void print_all_interfaces(const void *data)
{
	const struct rd *rd = (struct rd *) data;

	fprintf(stdout, "%-10s <%d>\n", rd->if_name, rd->if_flags);


	list_for_each(rd->ip_addr_list, print_all_addresses);

	return;
}

static int ifname_cmp(void *d1, void *d2)
{
	struct rd *rd = (struct rd *)d1;
	char *ifname = (char *)d2;

	return !strcmp(rd->if_name, ifname);
}

static int get_interface_addr(struct ospfd *ospfd)
{
	int ret;
	struct ifaddrs *ifaddr;

	/* First of all: remove all entries if any is available.
	 * This makes this method re-callable to refresh the interface address
	 * status */

	ret = getifaddrs(&ifaddr);
	if (ret < 0) {
		err_sys("failed to query interface addresses");
		return FAILURE;
	}

	while (ifaddr != NULL) {

		struct rd *rd;
		struct ip_addr *ip_addr;
		struct sockaddr_in *in4;
		struct sockaddr_in6 *in6;

		if (!ifaddr->ifa_addr)
			goto next;

		switch (ifaddr->ifa_addr->sa_family) { /* only IPv{4,6} */
			case AF_INET:
			case AF_INET6:
				break;
			default:
				goto next;
				break;
		}

		/* We know it is a IPv4 or IPv6 address.
		 * Now we search the routing domain list for
		 * this interface. If we found is we add the new
		 * IP address, if not we add a new routing domain
		 * and also add the IP address */
		struct list_e *list;
		list = list_search(ospfd->network.rd_list, ifname_cmp, ifaddr->ifa_name);
		if (list == NULL) { /* new interface ... */
			rd = xzalloc(sizeof(struct rd));
			ospfd->network.rd_list = list_insert_after(ospfd->network.rd_list, rd);
			memcpy(rd->if_name, ifaddr->ifa_name,
					min((strlen(ifaddr->ifa_name) + 1), sizeof(rd->if_name)));
			rd->if_flags  = ifaddr->ifa_flags; /* see netdevice(7) for list of flags */
			rd->ip_addr_list = list_create();
		} else {
			rd = list->data;
		}

		/* interface specific setup is fine, we can
		 * now carelessly add the new ip address to
		 * the interface */

		ip_addr = xzalloc(sizeof(struct ip_addr));
		ip_addr->family = ifaddr->ifa_addr->sa_family;
		switch (ip_addr->family) {
			case AF_INET:
				/* copy address */
				in4 = (struct sockaddr_in *)ifaddr->ifa_addr;
				memcpy(&ip_addr->ipv4.addr, &in4->sin_addr, sizeof(ip_addr->ipv4.addr));
				/* copy netmask */
				in4 = (struct sockaddr_in *)ifaddr->ifa_netmask;
				memcpy(&ip_addr->ipv4.netmask, &in4->sin_addr, sizeof(ip_addr->ipv4.netmask));
				/* copy broadcast */
				in4 = (struct sockaddr_in *)ifaddr->ifa_broadaddr;
				memcpy(&ip_addr->ipv4.broadcast, &in4->sin_addr, sizeof(ip_addr->ipv4.broadcast));
				break;
			case AF_INET6:
				/* copy address */
				in6 = (struct sockaddr_in6 *)ifaddr->ifa_addr;
				memcpy(&ip_addr->ipv6.addr, &in6->sin6_addr, sizeof(ip_addr->ipv6.addr));
				/* copy netmask */
				in6 = (struct sockaddr_in6 *)ifaddr->ifa_netmask;
				memcpy(&ip_addr->ipv6.netmask, &in6->sin6_addr, sizeof(ip_addr->ipv6.netmask));
				break;
			default:
				err_msg("Programmed error - address (protocol) not supported: %d",
						ip_addr->family);
				return FAILURE;
				break;
		}

		/* and at the newly data at the end of the list */
		rd->ip_addr_list = list_insert_before(rd->ip_addr_list, ip_addr);

next:
		ifaddr = ifaddr->ifa_next;

	}

	freeifaddrs(ifaddr);

	list_for_each(ospfd->network.rd_list, print_all_interfaces);

	return SUCCESS;
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

	ret = get_interface_addr(ospfd);
	if (ret != SUCCESS) {
		err_msg("failed to determine interface addresses");
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

	/* FIXME: add routine to free interface_address data - to late in the
	 * morning already -- some hours and my work starts ;-) */
}


/* vim: set tw=78 ts=4 sw=4 sts=4 ff=unix noet: */
