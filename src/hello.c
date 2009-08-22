#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ospfd.h"
#include "network.h"
#include "shared.h"
#include "buf.h"
#include "event.h"
#include "timer.h"
#include "hello.h"

#define	DEFAULT_MTU_SIZE 1500

uint16_t calc_csum(unsigned char *buf, ssize_t len)
{
	uint32_t sum;

	for (sum = 0; len > 0; len--) {
		sum += *buf++;
	}

	sum  = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);

	return ~sum;
}

static void ipv4_hdr_set_total_length(struct buf *packet_buffer, uint16_t length)
{
	struct iphdr *ip; char *c = buf_addr(packet_buffer);

	ip = (struct iphdr *) c;
	ip->tot_len = htons(length);

	return;
}

static void ipv4_set_csum(struct buf *packet_buffer)
{
	struct iphdr *ip = buf_addr(packet_buffer);

	ip->check = 0;
	ip->check = calc_csum((unsigned char *)ip, sizeof(struct iphdr));

	return;
}

static void hex_dump_ipv4_packet(struct buf *packet_buffer)
{
	size_t i;
	char *c;

	for (i = 0; i < buf_len(packet_buffer); i++) {
		c = buf_addr(packet_buffer);
		fprintf(stderr, "%02hhx ", c[i]);
		if (i != 0 && i % 16 == 0)
			fprintf(stderr, "\n");
	}
}


static int tx_prepare_ipv4_std_header(struct ospfd *ospfd,
		struct buf *packet_buffer, struct rc_rd *rc_rd)
{
	struct iphdr ip;
	char data[100] = { 0 };

	(void) ospfd;

	memset(&ip, 0, sizeof(struct ip));

	ip.version  = 4;
	ip.ihl      = sizeof(struct iphdr) >> 2;
	ip.tos      = 0x0;
	ip.id       = htons(getpid() & 0xFFFF);
	ip.frag_off = 0x0;
	ip.ttl      = 2;
	ip.protocol = IPPROTO_TCP;
	ip.check    = 0x0;

	/* TODO: this is more or less a little but specific ... */
	ip.saddr = rc_rd->ip_addr.ipv4.addr.s_addr;
	ip.daddr = inet_addr(MCAST_ALL_SPF_ROUTERS);

	buf_add(packet_buffer, (char *) &ip, sizeof(struct ip));
	buf_add(packet_buffer, data, 100);

	ipv4_hdr_set_total_length(packet_buffer, sizeof(struct iphdr) + 100);
	ipv4_set_csum(packet_buffer);

	hex_dump_ipv4_packet(packet_buffer);

	return SUCCESS;
}

static int tx_ipv4_buffer(const struct ospfd *ospfd, struct buf *packet_buffer)
{
	ssize_t ret;
	struct sockaddr_in sin;

	memset(&sin, 0, sizeof(struct sockaddr_in));

	sin.sin_family      = AF_INET;
	sin.sin_port        = 0;
	sin.sin_addr.s_addr = inet_addr(MCAST_ALL_SPF_ROUTERS);

	ret = sendto(ospfd->network.fd, buf_addr(packet_buffer),
			buf_len(packet_buffer), 0, (struct sockaddr *) &sin, sizeof (sin));
	if (ret < 0) {
		err_sys("cannot send HELLO message");
		return FAILURE;
	}

	return SUCCESS;
}

static int tx_prepare_ipv4_hello_msg(struct ospfd *ospfd, struct rc_rd *rc_rd)
{
	struct buf *packet_buffer;

	/* allocate a packet buffer - in the next couple
	 * of steps this buffer is increased and filled with
	 * data and finally pushed on the wire */
	packet_buffer = buf_alloc_hint(DEFAULT_MTU_SIZE);

	tx_prepare_ipv4_std_header(ospfd, packet_buffer, rc_rd);


	/* and finally send on the wire */
	tx_ipv4_buffer(ospfd, packet_buffer);


	buf_free(packet_buffer);

	return SUCCESS;
}

/* this function is called in regular intervals by the event loop */
void tx_ipv4_hello_packet(int fd, void *priv_data)
{
	int ret;
	struct tx_hello_arg *txha = priv_data;
	struct ospfd *ospfd = txha->ospfd;
	struct rc_rd *rc_rd = txha->rc_rd;

	msg(ospfd, VERBOSE, "generate new HELLO packet");

	/* first of all - disarm the timer_fd and do some
	 * sanity checks */
	ret = timer_del(ospfd, fd);
	if (ret != SUCCESS) {
		err_msg("failure in disamring the timer");
	}

	ret = tx_prepare_ipv4_hello_msg(ospfd, rc_rd);
	if (ret != SUCCESS) {
		err_msg("failed to create or transmit HELLO packet");
		return;
	}

	/* and rearm the timer */
	ret = timer_add_s_rel(ospfd, OSPF_DEFAULT_HELLO_INTERVAL,
			tx_ipv4_hello_packet, priv_data);
	if (ret != SUCCESS) {
		err_msg("Can't add timer for HELLO packet generation");
		return;
	}
}


/* vim: set tw=78 ts=4 sw=4 sts=4 ff=unix noet: */
