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
#include "hello_tx.h"
#include "rc.h"

static int list_cmp_interface_data(const void *a, const void *b)
{
	const struct interface_data *aa = a;
	const struct interface_data *bb = b;

	return !(strcmp(aa->if_name, bb->if_name));
}

static void list_free_interface_data(void *a)
{
	struct interface_data *aa = a;

	list_destroy(aa->neighbor_list);
	free(aa);
}


static struct ospfd *alloc_ospfd(void)
{
	struct ospfd *o;

	o = xzalloc(sizeof(struct ospfd));

	o->network.rd_list     = list_create(list_cmp_rd, list_free_rd);
	o->interface_data_list = list_create(list_cmp_interface_data, list_free_interface_data);

	return o;
}

static void free_ospfd(struct ospfd *o)
{
	free(o);
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

	/* seed pseudo number randon generator */
	init_pnrg(ospfd);

	/* initialize event subsystem. In this case this belongs
	 * to open a epoll filedescriptor */
	ret = ev_init(ospfd);
	if (ret != SUCCESS)
		err_msg_die(EXIT_FAILURE, "Can't initialize event subsystem");


	ret = parse_rc_file(ospfd);
	if (ret != SUCCESS)
		err_msg_die(EXIT_FAILURE, "Can't parse configuration file");


	ret = init_network(ospfd);
	if (ret != SUCCESS)
		err_msg_die(EXIT_FAILURE, "Can't initialize network subsystem");

	/* and branch into the main loop
	 * This loop will never return (with the exception of SIGINT or failure
	 * condition) */
	ret = ev_loop(ospfd);
	if (ret != SUCCESS)
		err_msg_die(EXIT_FAILURE, "Main loop returned unexpected - exiting now");


	fini_network(ospfd);
	free_options(ospfd);
	free_ospfd(ospfd);

	return EXIT_SUCCESS;
}


/* vim: set tw=78 ts=4 sw=4 sts=4 ff=unix noet: */
