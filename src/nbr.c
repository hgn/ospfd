#include <stdio.h>
#include <stdlib.h>

#include "shared.h"
#include "hello.h"
#include "timer.h"
#include "nbr.h"

struct rc_rd *alloc_rc_rd(void)
{
	struct rc_rd *rc_rd;

	rc_rd = xzalloc(sizeof(struct rc_rd));

	/* set default values for the interface */
	rc_rd->state = INF_STATE_DOWN;

	return rc_rd;
}

static void s_down_rx_up(struct ospfd *ospfd, struct rc_rd *rc_rd)
{
	int hello_start, ret;
	struct tx_hello_arg *txha;

	/* the interface was down and is now re-enabled -> start
	 * HELLO timer */
	txha = xzalloc(sizeof(struct tx_hello_arg));
	txha->ospfd = ospfd;
	txha->rc_rd = rc_rd;

	/* should we delay the first start a little bit? Is e.g. 1 second - a user
	 * selected variable - to short after the daemon is "hopefully" good
	 * natured startet. Should it code add a small offset? */
	hello_start = rc_rd->hello_interval ?
		rc_rd->hello_interval : OSPF_DEFAULT_HELLO_INTERVAL;

	/* initialize timer for regular HELLO packet transmission */
	ret = timer_add_s_rel(ospfd, hello_start, tx_ipv4_hello_packet, txha);
	if (ret != SUCCESS) {
		err_msg("Can't add timer for HELLO packet generation");
		return;
	}

	return;
}

static void process_in_down(struct ospfd *ospfd,
		struct rc_rd *rc_rd, int new_state)
{
	switch (new_state) {
	case INF_EV_INTERFACE_UP:
		s_down_rx_up(ospfd, rc_rd);
		break;
	default:
		err_msg_die(EXIT_FAILURE, "Programmed error in switch/case statement: "
			"state (%d) not known or not handled", rc_rd->state);
		abort();
		break;
	}
}

void nbr_set_state(struct ospfd *ospfd,
		struct rc_rd *rc_rd, int new_state)
{
	switch (rc_rd->state) {
	case INF_STATE_DOWN:
		process_in_down(ospfd, rc_rd, new_state);
		break;
	default:
		err_msg_die(EXIT_FAILURE, "Programmed error in switch/case statement: "
			"state (%d) not known or not handled", rc_rd->state);
		abort();
		break;
	}
}

/* vim: set tw=78 ts=4 sw=4 sts=4 ff=unix noet: */
