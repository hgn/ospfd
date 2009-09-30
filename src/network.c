#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <net/if.h>

#include <sys/types.h>
#include <sys/socket.h>

#include "network.h"
#include "shared.h"
#include "interface.h"

/* returns true if both interfaces
 * are identical */
int list_cmp_rd(const void *a, const void *b)
{
	const struct rd *aa = a;
	const struct rd *bb = b;

	return aa->if_index == bb->if_index;
}

void list_free_rd(void *a)
{
	struct rd *rd = (struct rd *) a;

	list_destroy(rd->ip_addr_list);
	free(rd);
}


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
	int fd, on = 1, ret, flags;

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

	/* make socket non-blocking */
	if ((flags = fcntl(fd, F_GETFL, 0)) == -1)
		flags = 0;
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);


	ospfd->network.fd = fd;

	return SUCCESS;
}

static void fini_raw_socket(struct ospfd *ospfd)
{
	close(ospfd->network.fd);
}

void free_ip_addr(void *ip_addr)
{
	free(ip_addr);
}

/* return true if both addresses are equal - otherwise false */
int list_cmp_struct_ip_addr(const void *a, const void *b)
{
	const struct ip_addr *aa = (struct ip_addr *) a;
	const struct ip_addr *bb = (struct ip_addr *) b;

	if (aa->family != bb->family)
		return 0;

	switch (aa->family) {

		case AF_INET:
			if (aa->ipv4.addr.s_addr      == bb->ipv4.addr.s_addr    &&
			    aa->ipv4.netmask.s_addr   == bb->ipv4.netmask.s_addr &&
			    aa->ipv4.broadcast.s_addr == bb->ipv4.broadcast.s_addr)
				return 1;
			else
				return 0;
			break;

		case AF_INET6:
			if (!(memcmp(&aa->ipv6.addr, &bb->ipv6.addr, sizeof(struct in6_addr)))       &&
			    !(memcmp(&aa->ipv6.netmask, &bb->ipv6.netmask, sizeof(struct in6_addr))) &&
			     (aa->ipv6.scope == bb->ipv6.scope))
				return 1;
			else
				return 0;
		default:
			/* dont know how to compare */
			abort();
	}
}

#if 0

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

#endif

void print_all_interfaces(const void *data)
{
	const struct rd *rd = (struct rd *) data;

	fprintf(stdout, "%-10s <%d>\n", rd->if_name, rd->if_flags);

	/* list_for_each(rd->ip_addr_list, print_all_addresses); */

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
	 * status.  */
	list_destroy(ospfd->network.rd_list);
	ospfd->network.rd_list = list_create(list_cmp_rd, list_free_rd);

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
		rd = list_lookup_match(ospfd->network.rd_list, ifname_cmp, ifaddr->ifa_name);
		if (rd == NULL) { /* new interface ... */
			rd = xzalloc(sizeof(struct rd));
			ret = list_ins_next(ospfd->network.rd_list, NULL, rd);
			if (ret != SUCCESS) {
				fprintf(stderr, "Failure in inserting rd\n");
				abort();
			}
			memcpy(rd->if_name, ifaddr->ifa_name,
					min((strlen(ifaddr->ifa_name) + 1), sizeof(rd->if_name)));
			rd->if_flags  = ifaddr->ifa_flags; /* see netdevice(7) for list of flags */
			rd->ip_addr_list = list_create(list_cmp_struct_ip_addr, free_ip_addr);
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
				/* and scope too */
				ip_addr->ipv6.scope = in6->sin6_scope_id;
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
		list_insert(rd->ip_addr_list, ip_addr);

next:
		ifaddr = ifaddr->ifa_next;

	}

	freeifaddrs(ifaddr);

#if 0
	list_for_each(ospfd->network.rd_list, print_all_interfaces);
#endif

	return SUCCESS;
}

/* Fletcher Checksum - RFC 1008 */
uint16_t calc_fl_checksum(char *buf, uint16_t pos, uint16_t len)
{
	char *data = buf;
	int c0 = 0, c1 = 0, x;
	uint16_t offset;

	offset = len - pos - 1;

	/* skip age field */
	data += 2;
	len -= 2;

	while (len--) {
		c0 += *data++;
		c1 += c0;
		if ((len & 0xfff) == 0) {
			c0 %= 255;
			c1 %= 255;
		}
	}

	/* ! validata mode */
	if (pos) {
		x = (int)(offset * c0 - c1) % 255;

		if (x <= 0) {
			x += 255;
		}
		c1 = 510 - c0 - x;
		if (c1 > 255) {
			c1 -= 255;
		}
		c0 = x;
	}

	return htons((c0 << 8) | (c1 & 0xff));
}


int init_network(struct ospfd *ospfd)
{
	int ret;
	struct ev_entry *ee;

	switch (ospfd->opts.family) {
		case AF_INET:
			ret = init_raw_4_socket(ospfd);
			if (ret != SUCCESS) {
				err_msg("Failed to initialize raw socket");
				return FAILURE;
			}
			/* and register fd */
			ee = ev_entry_new(ospfd->network.fd, EV_READ, packet_input, ospfd);
			if (!ee) {
				err_msg("cannot at RAW socket to event mechanism");
				return FAILURE;
			}
			ret = ev_add(ospfd->ev, ee);
			if (ret != EV_SUCCESS) {
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

void init_o_buf(struct o_buf *o_buf)
{
	memset(o_buf, 0, sizeof(struct o_buf));

	o_buf->ifindex = O_BUF_INF_UNKOWN;
}


/* vim: set tw=78 ts=4 sw=4 sts=4 ff=unix noet: */
