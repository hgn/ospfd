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
#include "hello_rx.h"

int hello_ipv4_in(struct ospfd *ospfd,
		char *packet, ssize_t packet_len)
{
	const struct hello_ipv4_std_header *hdr;

	hdr = (struct hello_ipv4_std_header *) packet;


	return SUCCESS;
}



/* vim: set tw=78 ts=4 sw=4 sts=4 ff=unix noet: */
