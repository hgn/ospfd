#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "shared.h"
#include "hello_tx.h"
#include "interface.h"
#include "neighbor.h"

struct neighbor *alloc_neighbor(void)
{
	struct neighbor *neighbor;

	neighbor = xzalloc(sizeof(struct neighbor));

	neighbor->state = NEIGHBOR_STATE_DOWN;

	return neighbor;
}

void interface_add_neighbor(struct interface_data *interface_data, struct neighbor *neighbor)
{
	int ret;

	ret = list_insert(interface_data->neighbor_list, neighbor);
	if (ret != SUCCESS) {
		fprintf(stderr, "Cannot insert neigbor into list\n");
		abort();
	}
}

/* it is up to the callee to remove this neighbor out
 * of any lists */
void free_neighbor(void *a)
{
	struct neighbor *neighbor = a;
	memset(neighbor, 0, sizeof(struct neighbor));
	free(neighbor);
}

static void neighbor_set_state(struct ospfd *ospfd, struct interface_data *interface_data,
		struct neighbor *neighbor, int new_state)
{
	neighbor->state = new_state;
}

void neighbor_inactive_timer_expired(void *data)
{
	struct inactivity_timer_data *inactivity_timer_data = data;
	struct ospfd *ospfd = inactivity_timer_data->ospfd;
	struct neighbor *neighbor = inactivity_timer_data->neighbor;
	struct interface_data *interface_data = inactivity_timer_data->interface_data;

	msg(ospfd, DEBUG, "neighbor inactive timer expired (router id: %d)",
			neighbor->neighbor_id);

	/* remove neighbor from interface list */
	remove_neighbor_from_interface_data_list(ospfd, interface_data, neighbor);

	free(neighbor->inactivity_timer_data);
	free(neighbor);
}

int neighbor_start_inactive_timer(struct ospfd *ospfd, struct interface_data *i, struct neighbor *neighbor)
{
	int ret;
	struct inactivity_timer_data *inactivity_timer_data;
	/* FIXME: hardcoded time-out */
	struct timespec timespec = { 10, 0 };
	struct ev_entry *ev_entry;

	msg(ospfd, DEBUG, "prepare inactive timer for neighbor");

	inactivity_timer_data = xzalloc(sizeof(struct inactivity_timer_data));
	inactivity_timer_data->ospfd    = ospfd;
	inactivity_timer_data->neighbor = neighbor;
	inactivity_timer_data->interface_data = i;

	neighbor->inactivity_timer_data = inactivity_timer_data;

	ev_entry = ev_timer_new(&timespec, neighbor_inactive_timer_expired, inactivity_timer_data);
	if (!ev_entry) {
		err_msg_die(EXIT_FAILURE, "Cannot initialize a new timer");
	}

	inactivity_timer_data->neighbor->inactive_timer_entry = ev_entry;

	ret = ev_add(ospfd->ev, ev_entry);
	if (ret != EV_SUCCESS) {
		err_msg_die(EXIT_FAILURE, "Cannot add new timer to global event handler");
	}

	return SUCCESS;
}

int neighbor_cancel_inactive_timer(struct ospfd *ospfd, struct neighbor *neighbor)
{
	assert(ospfd->ev);
	assert(neighbor);
	assert(neighbor->inactive_timer_entry);

	msg(ospfd, DEBUG, "cancel neighbor inactive timer");

	ev_timer_cancel(ospfd->ev, neighbor->inactive_timer_entry);

	free(neighbor->inactivity_timer_data);

	return SUCCESS;
}

int neighbor_restart_inactive_timer(struct ospfd *ospfd, struct interface_data *interface_data, struct neighbor *neighbor)
{
	int ret;

	/* stop running inactivity timer ... */
	ret = neighbor_cancel_inactive_timer(ospfd, neighbor);
	if (ret != SUCCESS)
		return FAILURE;

	/* ... and re-arm the timer again! */
	ret = neighbor_start_inactive_timer(ospfd, interface_data, neighbor);
	if (ret != SUCCESS)
		return FAILURE;

	return SUCCESS;
}

/* returns true if the neighbor id is identical
 * and false in all other cases */
int list_neighbor_id_cmp(const void *a, const void *b)
{
	const struct neighbor *aa = (struct neighbor *)a;
	const struct neighbor *bb = (struct neighbor *)b;

	return aa->neighbor_id == bb->neighbor_id;
}


static int neighbor_id_cmp(void *a, void *b)
{
	struct neighbor *n = (struct neighbor *)a;
	uint32_t *id = b;

	return n->neighbor_id == *id;
}


/* for a given interface data and neighbor id this
 * function returns the neighbor data structure or
 * NULL in the cases that these neighbor is not in
 * the interface data structure */
struct neighbor *neighbor_by_id(struct ospfd *ospfd,
		struct interface_data *interface_data, uint32_t neighbor_id)
{
	struct neighbor *neighbor;

	(void) ospfd;

	neighbor = list_lookup_match(interface_data->neighbor_list,
			neighbor_id_cmp, &neighbor_id);

	return neighbor ? neighbor : NULL;
}

void remove_neighbor_from_interface_data_list(struct ospfd *ospfd,
		struct interface_data *interface_data, struct neighbor *neighbor)
{
	int ret;

	/* this impliciet calls the compare function of
	 * interface_data->neighbor_list */
	ret = list_remove(interface_data->neighbor_list, (void **)&neighbor);
	if (ret != SUCCESS) {
		fprintf(stderr, "Cannot remove neigbor from list\n");
		abort();
	}
}

int process_state_down_ev_hello_received(struct ospfd *ospfd,
		struct interface_data *i, struct neighbor *neighbor)
{
	int ret;

	/* set state to INIT */
	neighbor_set_state(ospfd, i, neighbor, NEIGHBOR_STATE_INIT);

	ret = neighbor_start_inactive_timer(ospfd, i, neighbor);
	if (ret != SUCCESS)
		return FAILURE;

	return SUCCESS;
}

int process_state_init_ev_hello_received(struct ospfd *o, struct interface_data *i,
		struct neighbor *n, int new_state)
{
	int ret;

	ret = neighbor_restart_inactive_timer(o, i, n);
	if (ret != SUCCESS)
		return FAILURE;

	return SUCCESS;
}


int process_state_down_to_new_state(struct ospfd *o, struct interface_data *i,
		struct neighbor *n, int event)
{
	switch (event) {
		case NEIGHBOR_EV_HELLO_RECEIVED:
			return process_state_down_ev_hello_received(o, i, n);
			break;
		default:
			err_msg_die(EXIT_FAILURE, "State no handled: %s:%d",
					__FILE__, __LINE__);
			break;
	}

	/* should not happened - to shut-up gcc warnings */
	return FAILURE;
}

int process_state_init_to_new_state(struct ospfd *o, struct interface_data *i,
		struct neighbor *n, int s)
{
	switch (s) {
		case NEIGHBOR_EV_HELLO_RECEIVED:
			return process_state_init_ev_hello_received(o, i, n, s);
			break;
		default:
			err_msg_die(EXIT_FAILURE, "State no handled: %s:%d",
					__FILE__, __LINE__);
			break;
	}

	/* should not happened - to shut-up gcc warnings */
	return FAILURE;
}


void process_state_two_way_to_new_state(struct ospfd *ospfd,
		struct neighbor *neighbor, int new_state)
{
	err_msg_die(EXIT_FAILURE, "State handling not yet implemented: %s:%d",
			__FILE__, __LINE__);
}


void process_state_ex_start_to_new_state(struct ospfd *ospfd,
		struct neighbor *neighbor, int new_state)
{
	err_msg_die(EXIT_FAILURE, "State handling not yet implemented: %s:%d",
			__FILE__, __LINE__);
}


void process_state_exchange_to_new_state(struct ospfd *ospfd,
		struct neighbor *neighbor, int new_state)
{
	err_msg_die(EXIT_FAILURE, "State handling not yet implemented: %s:%d",
			__FILE__, __LINE__);
}


void process_state_loading_to_new_state(struct ospfd *ospfd,
		struct neighbor *neighbor, int new_state)
{
	err_msg_die(EXIT_FAILURE, "State handling not yet implemented: %s:%d",
			__FILE__, __LINE__);
}


void process_state_full_to_new_state(struct ospfd *ospfd,
		struct neighbor *neighbor, int new_state)
{
	err_msg_die(EXIT_FAILURE, "State handling not yet implemented: %s:%d",
			__FILE__, __LINE__);
}


int neighbor_process_event(struct ospfd *ospfd, struct interface_data *interface_data,
		struct neighbor *neighbor, int event)
{
	switch (neighbor->state) {
		case NEIGHBOR_STATE_DOWN:
			return process_state_down_to_new_state(ospfd, interface_data, neighbor, event);
			break;
		case NEIGHBOR_STATE_INIT:
			return process_state_init_to_new_state(ospfd, interface_data, neighbor, event);
			break;
		case NEIGHBOR_STATE_ATTEMPT:
		case NEIGHBOR_STATE_TWO_WAY:
		case NEIGHBOR_STATE_EX_START:
		case NEIGHBOR_STATE_EXCHANGE:
		case NEIGHBOR_STATE_LOADING:
		case NEIGHBOR_STATE_FULL:
		default:
			err_msg_die(EXIT_FAILURE, "Programmed error in switch/case statement: "
					"state (%d) not known or not handled", neighbor->state);
			break;
	};

	return FAILURE;
}

/* vim: set tw=78 ts=4 sw=4 sts=4 ff=unix noet: */
