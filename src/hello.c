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

/* this function is called in regular intervals by the event loop */
void tx_hello_packet(int fd, void *priv_data)
{
	int ret;
	int64_t time_buf;
	struct ospfd *ospfd = priv_data;
	fprintf(stderr,"generate HELLO packet\n");

	/* first of all - disarm the timer_fd and do some
	 * sanity checks */
	ret = read(fd, &time_buf, sizeof(int64_t));
	if (ret < (int)sizeof(int64_t)) {
		err_msg("Cannot proper read %d bytes", sizeof(int64_t));
		return;
	}
	if (time_buf > 1)
		fprintf(stderr, "Timer was not proper handled");



	/* and rearm the timer */
	ret = timer_add_s_rel(ospfd, OSPF_DEFAULT_HELLO_INTERVAL, tx_hello_packet, ospfd);
	if (ret != SUCCESS) {
		err_msg("Can't add timer for HELLO packet generation");
		return;
	}

}


/* vim: set tw=78 ts=4 sw=4 sts=4 ff=unix noet: */
