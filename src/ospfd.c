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
#include "rc.h"


static struct ospfd *alloc_ospfd(void)
{
	struct ospfd *o;

	o = xzalloc(sizeof(struct ospfd));

	o->network.rd_list = list_create();
	o->rc_rd_list      = list_create();

	return o;
}

static void free_ospfd(struct ospfd *o)
{
	free(o);
}


static void init_standard_timers(void *d1, void *d2)
{
	int ret;
	struct rc_rd *rc_rd = (struct rc_rd *) d1;
	struct ospfd *ospfd = (struct ospfd *) d2;
	struct tx_hello_arg *txha;

	txha = xzalloc(sizeof(struct tx_hello_arg));
	txha->ospfd = ospfd;
	txha->rc_rd = rc_rd;

	/* initialize timer for regular HELLO packet transmission */
	ret = timer_add_s_rel(ospfd, OSPF_DEFAULT_HELLO_INTERVAL, tx_ipv4_hello_packet, txha);
	if (ret != SUCCESS) {
		err_msg("Can't add timer for HELLO packet generation");
		return;
	}

	return;
}

static int initiate_exchange(struct ospfd *ospfd)
{
	/* for each configure interface one timer */
	list_for_each_with_arg(ospfd->rc_rd_list, init_standard_timers, ospfd);

	return SUCCESS;
}


int main(int ac, char **av)
{
	int ret;
	struct ospfd *ospfd;

	ospfd = alloc_ospfd();

	ret = parse_cli_options(ospfd, ac, av);
	if (ret != SUCCESS)
		err_msg_die(EXIT_FAILURE, "Can't parse command line");

	msg(ospfd, GENTLE, PROGRAMNAME " - " VERSIONSTRING);

	ret = parse_rc_file(ospfd);
	if (ret != SUCCESS)
		err_msg_die(EXIT_FAILURE, "Can't parse configuration file");

	/* initialize event subsystem. In this case this belongs
	 * to open a epoll filedescriptor */
	ret = ev_init(ospfd);
	if (ret != SUCCESS)
		err_msg_die(EXIT_FAILURE, "Can't initialize event subsystem");


	ret = init_network(ospfd);
	if (ret != SUCCESS)
		err_msg_die(EXIT_FAILURE, "Can't initialize network subsystem");

	/* initialize for every configure interface a HELLO interval */
	ret = initiate_exchange(ospfd);
	if (ret != SUCCESS)
		err_msg_die(EXIT_FAILURE, "Failed to setup standard timers - exiting now");


	/* and branch into the main loop
	 * This loop will return (with the exception of SIGINT or failure
	 * condition) */
	ret = ev_loop(ospfd);
	if (ret != SUCCESS)
		err_msg_die(EXIT_FAILURE, "Main loop returned unexpected - exiting now");


	fini_network(ospfd);
	free_ospfd(ospfd);

	return EXIT_SUCCESS;
}


/* vim: set tw=78 ts=4 sw=4 sts=4 ff=unix noet: */
