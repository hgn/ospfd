#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shared.h"
#include "hello_tx.h"
#include "timer.h"
#include "interface.h"

int interface_data_name_cmp(void *d1, void *d2)
{
	struct interface_data *id = (struct interface_data *) d1;
	char *ifname = (char *)d2;

	return !strcmp(id->if_name, ifname);
}

struct interface_data *interface_data_for_index(const struct ospfd *ospfd,
		unsigned int infindex)
{
	char ifname[IF_NAMESIZE + 1], *cptr;
	struct list_e *list;
	struct list_e *interface_data_list;

	cptr = ifname;

	cptr = if_indextoname(infindex, ifname);
	if (!cptr) {
		return NULL;
	}

	/* search routing interface for this interface name */
	interface_data_list = ospfd->interface_data_list;

	list = list_search(interface_data_list, interface_data_name_cmp, ifname);

	return list ? list->data : NULL;
}

struct interface_data *alloc_interface_data(void)
{
	struct interface_data *interface_data;

	interface_data = xzalloc(sizeof(struct interface_data));

	/* set default values for the interface */
	interface_data->state                = INF_STATE_DOWN;
	interface_data->router_priority      = OSPF_DEFAULT_ROUTER_PRIORITY;
	interface_data->router_dead_interval = OSPF_DEFAULT_ROUTER_DEAD_INTERVAL;
	interface_data->type                 = INF_TYPE_UNKNOWN;

	/* initialize the list of (hopefully) upcoming neighbors
	   for this interface */
	interface_data->neighbor_list = list_create();

	return interface_data;
}

static void s_down_rx_up(struct ospfd *ospfd, struct interface_data *interface_data)
{
	int hello_start, ret;
	struct tx_hello_arg *txha;

	/* the interface was down and is now re-enabled -> start
	 * HELLO timer */
	txha = xzalloc(sizeof(struct tx_hello_arg));
	txha->ospfd = ospfd;
	txha->interface_data = interface_data;

	/* should we delay the first start a little bit? Is e.g. 1 second - a user
	 * selected variable - to short after the daemon is "hopefully" good
	 * natured startet. Should it code add a small offset? */
	hello_start = interface_data->hello_interval ?
		interface_data->hello_interval : OSPF_DEFAULT_HELLO_INTERVAL;

	/* initialize timer for regular HELLO packet transmission */
	ret = timer_add_s_rel(ospfd, hello_start, tx_ipv4_hello_packet, txha);
	if (ret != SUCCESS) {
		err_msg("Can't add timer for HELLO packet generation");
		return;
	}

	return;
}

static void process_in_down(struct ospfd *ospfd,
		struct interface_data *interface_data, int new_state)
{
	switch (new_state) {
	case INF_EV_INTERFACE_UP:
		s_down_rx_up(ospfd, interface_data);
		break;
	default:
		err_msg_die(EXIT_FAILURE, "Programmed error in switch/case statement: "
			"state (%d) not known or not handled", interface_data->state);
		abort();
		break;
	}
}

void interface_set_state(struct ospfd *ospfd,
		struct interface_data *interface_data, int new_state)
{
	switch (interface_data->state) {
	case INF_STATE_DOWN:
		process_in_down(ospfd, interface_data, new_state);
		break;
	default:
		err_msg_die(EXIT_FAILURE, "Programmed error in switch/case statement: "
			"state (%d) not known or not handled", interface_data->state);
		abort();
		break;
	}
}


static int neighbor_id_cmp(void *a, void *b)
{
	struct neighbor *n = (struct neighbor *)a;
	uint32_t *id = b;

	return n->neighbor_id == *id;
}


struct neighbor *neighbor_by_id(struct ospfd *ospfd,
		struct interface_data *interface_data, uint32_t neighbor_id)
{
	struct list_e *list;
	(void) ospfd;

	list = list_search(interface_data->neighbor_list,
			neighbor_id_cmp, &neighbor_id);

	return list ? list->data : NULL;
}

/* vim: set tw=78 ts=4 sw=4 sts=4 ff=unix noet: */
