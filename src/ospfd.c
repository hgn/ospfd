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


static struct ospfd *alloc_ospfd(void)
{
	return xzalloc(sizeof(struct ospfd));
}

static void free_ospfd(struct ospfd *o)
{
	free(o);
}

static void set_default_options(struct opts *opts)
{
	opts->family        = AF_INET;
	opts->verbose_level = GENTLE;
}

int parse_options(struct opts *opts, int ac, char **av)
{
	set_default_options(opts);

	opts->me = xstrdup(av[0]);

	return SUCCESS;
}

static int init_standard_timers(struct ospfd *ospfd)
{
	int ret;
	/* initialize timer for regular HELLO packet transmission */
	ret = timer_add_s_rel(ospfd, OSPF_DEFAULT_HELLO_INTERVAL, tx_hello_packet, ospfd);
	if (ret != SUCCESS) {
		err_msg("Can't add timer for HELLO packet generation");
		return FAILURE;
	}

	return SUCCESS;
}


int main(int ac, char **av)
{
	int ret;
	struct ospfd *ospfd;

	ospfd = alloc_ospfd();

	msg(ospfd, GENTLE, PROGRAMNAME " - " VERSIONSTRING);

	ret = parse_options(&ospfd->opts, ac, av);
	if (ret != SUCCESS)
		err_msg_die(EXIT_FAILURE, "Can't parse command line");

	/* initialize event subsystem. In this case this belongs
	 * to open a epoll filedescriptor */
	ret = ev_init(ospfd);
	if (ret != SUCCESS)
		err_msg_die(EXIT_FAILURE, "Can't initialize event subsystem");


	ret = init_network(ospfd);
	if (ret != SUCCESS)
		err_msg_die(EXIT_FAILURE, "Can't initialize network subsystem");

	ret = init_standard_timers(ospfd);
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
