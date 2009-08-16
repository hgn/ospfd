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
#include "buf.h"
#include "event.h"
#include "timer.h"
#include "hello.h"

static struct buf *prepare_ipv4_std_header(struct ospfd *ospfd)
{

}

static int prepare_and_tx_ipv4_hello_msg(struct ospfd *ospfd)
{
	int ret = SUCCESS;

	return ret;
}

/* this function is called in regular intervals by the event loop */
void tx_ipv4_hello_packet(int fd, void *priv_data)
{
	int ret;
	struct ospfd *ospfd = priv_data;

	msg(ospfd, LOUDISH, "generate new HELLO packet");

	/* first of all - disarm the timer_fd and do some
	 * sanity checks */
	ret = timer_del(ospfd, fd);
	if (ret != SUCCESS) {
		err_msg("failure in disamring the timer");
	}

	ret = prepare_and_tx_ipv4_hello_msg(ospfd);
	if (ret != SUCCESS) {
		err_msg("failed to create or transmit HELLO packet");
		return;
	}

	/* and rearm the timer */
	ret = timer_add_s_rel(ospfd, OSPF_DEFAULT_HELLO_INTERVAL, tx_ipv4_hello_packet, ospfd);
	if (ret != SUCCESS) {
		err_msg("Can't add timer for HELLO packet generation");
		return;
	}
}


/* vim: set tw=78 ts=4 sw=4 sts=4 ff=unix noet: */
