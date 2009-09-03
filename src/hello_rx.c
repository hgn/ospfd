#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "ospfd.h"
#include "shared.h"
#include "buf.h"
#include "event.h"
#include "timer.h"
#include "hello_rx.h"
#include "interface.h"

int hello_ipv4_in(struct ospfd *ospfd, struct o_buf *o_buf)
{
	struct interface_data *inf_data;

	/* get the right interface structure for the current
	 * incoming packet */
	inf_data = interface_data_for_index(ospfd, o_buf->ifindex);
	if (!inf_data) {
		msg(ospfd, VERBOSE, "Cannot determine interface data for incoming HELLO packet."
				" This probably means that this interface was not configured for routing but "
				" one peer node on the other end of this interface transmitted a HELLO packet");
		return FAILURE;
	}


	return SUCCESS;
}



/* vim: set tw=78 ts=4 sw=4 sts=4 ff=unix noet: */
