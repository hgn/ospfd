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
#include "hello_rx.h"
#include "interface.h"
#include "neighbor.h"

static int save_hello_data_for_neighbor(struct ospfd *ospfd,
		struct o_buf *o_buf, struct neighbor *neighbor)
{
	struct ipv4_ospf_header *ipv4_ospf_header   = o_buf->ospf_hdr.ipv4_ospf_header;
	struct ipv4_hello_header *ipv4_hello_header = o_buf->data_hdr.ipv4_hello_header;

	neighbor->neighbor_id = ntohl(ipv4_ospf_header->router_id);

	msg(ospfd, DEBUG, "update neighbor information (id: %d) from HELLO packet",
			neighbor->neighbor_id);

	return SUCCESS;
}

int hello_ipv4_in(struct ospfd *ospfd, struct o_buf *o_buf)
{
	uint32_t neighbor_id;
	struct interface_data *inf_data;
	struct neighbor *neighbor;

	/* get the right interface structure for the current
	 * incoming packet */
	inf_data = interface_data_for_index(ospfd, o_buf->ifindex);
	if (!inf_data) {
		msg(ospfd, VERBOSE, "Cannot determine interface data for incoming HELLO packet."
				" This probably means that this interface was not configured for routing but "
				" one peer node on the other end of this interface transmitted a HELLO packet");
		return FAILURE;
	}

	neighbor_id = ntohl(o_buf->ospf_hdr.ipv4_ospf_header->router_id);

	/* Nice! Now we check if the current HELLO
	 * packet is a new neighbor or an already known
	 * one. In both cases we will update the required
	 * fields - in one case more, in the other case less */
	neighbor = neighbor_by_id(ospfd, inf_data, neighbor_id);
	if (neighbor) {
		msg(ospfd, DEBUG, "received HELLO packet - neighbor was already known");
		neighbor_process_event(ospfd, inf_data, neighbor, NEIGHBOR_EV_HELLO_RECEIVED);
	} else { /* new neighbor - welcome! */
		msg(ospfd, DEBUG, "received HELLO packet - neighbor is new");
		neighbor = alloc_neighbor();
		save_hello_data_for_neighbor(ospfd, o_buf, neighbor);
		interface_add_neighbor(inf_data, neighbor);
		neighbor_process_event(ospfd, inf_data, neighbor, NEIGHBOR_EV_HELLO_RECEIVED);
	}



	return SUCCESS;
}



/* vim: set tw=78 ts=4 sw=4 sts=4 ff=unix noet: */
