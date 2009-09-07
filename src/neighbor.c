#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "shared.h"
#include "hello_tx.h"
#include "timer.h"
#include "interface.h"
#include "neighbor.h"

struct neighbor *alloc_neighbor(void)
{
	struct neighbor *neighbor;

	neighbor = xzalloc(sizeof(struct neighbor));

	neighbor->state = NEIGHBOR_STATE_DOWN;
	neighbor->inactive_timer = NEIGHBOR_INACTIVE_TIMER_DISABLED;

	return neighbor;
}

void interface_add_neighbor(struct interface_data *interface_data, struct neighbor *neighbor)
{
	interface_data->neighbor_list = list_insert_before(interface_data->neighbor_list, neighbor);
}

void free_neighbor(struct ospfd *ospfd, struct neighbor *neighbor)
{
	/* XXX: is MUST be no race between the timer cancelization
	 * and the freeing of ressources of this neighbor. */
	neighbor_cancel_inactive_timer(ospfd, neighbor);
	free(neighbor);
}

static void neighbor_set_state(struct ospfd *ospfd, struct interface_data *interface_data,
		struct neighbor *neighbor, int new_state)
{
	neighbor->state = new_state;
}

void neighbor_inactive_timer_expired(int fd, void *data)
{
	struct inactivity_timer_data *inactivity_timer_data = data;
	struct ospfd *ospfd = inactivity_timer_data->ospfd;
	struct neighbor *neighbor = inactivity_timer_data->neighbor;
	struct interface_data *interface_data = inactivity_timer_data->interface_data;

	assert(fd == neighbor->inactive_timer);

	msg(ospfd, DEBUG, "neighbor inactive timer expired (router id: %d)",
			neighbor->neighbor_id);

	timer_del(ospfd, neighbor->inactive_timer);

	/* remove neighbor from interface list */
	remove_neighbor_from_interface_data_list(ospfd, interface_data, neighbor);

	free(neighbor->inactivity_timer_data);
	free(neighbor->timer_priv_data);
	free(neighbor);
}

int neighbor_start_inactive_timer(struct ospfd *ospfd, struct interface_data *i, struct neighbor *neighbor)
{
	int ret;
	struct inactivity_timer_data *inactivity_timer_data;

	msg(ospfd, DEBUG, "prepare inactive timer for neighbor");

	inactivity_timer_data = xzalloc(sizeof(struct inactivity_timer_data));
	inactivity_timer_data->ospfd    = ospfd;
	inactivity_timer_data->neighbor = neighbor;
	inactivity_timer_data->interface_data = i;

	neighbor->inactivity_timer_data = inactivity_timer_data;

	/* FIXME: hardcoded 5 seconds time-out */
	ret = timer_add_s_rel_sovereignty(ospfd, 1, neighbor_inactive_timer_expired, inactivity_timer_data, &neighbor->timer_priv_data, &neighbor->inactive_timer);

	return SUCCESS;
}

int neighbor_cancel_inactive_timer(struct ospfd *ospfd, struct neighbor *neighbor)
{
	msg(ospfd, DEBUG, "cancel neighbor inactive timer (fd: %d)", neighbor->inactive_timer);

	timer_del_early(ospfd, neighbor->inactive_timer);

	free(neighbor->timer_priv_data);
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

void remove_neighbor_from_interface_data_list(struct ospfd *ospfd,
		struct interface_data *interface_data, struct neighbor *neighbor)
{
	struct list_e *list;

	list = list_search(interface_data->neighbor_list,
			neighbor_id_cmp, &neighbor->neighbor_id);
	if (!list) {
		msg(ospfd, VERBOSE, "List element does not exist!? %s:%d",
				__FILE__, __LINE__);
		return;
	}

	interface_data->neighbor_list = list_delete_element(list);
}

int process_state_down_ev_hello_received(struct ospfd *ospfd,
		struct interface_data *i, struct neighbor *neighbor)
{
	int ret;

	/* this is a new neighbor - there must be no running
	 * inactivity timer for this neighbor */
	assert(neighbor->inactive_timer == NEIGHBOR_INACTIVE_TIMER_DISABLED);

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

	assert(n->inactive_timer != NEIGHBOR_INACTIVE_TIMER_DISABLED);

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
