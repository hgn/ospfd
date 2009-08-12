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
	opts->family = AF_INET;
}

int parse_options(struct opts *opts, int ac, char **av)
{
	set_default_options(opts);

	opts->me = xstrdup(av[0]);

	return SUCCESS;
}


int main(int ac, char **av)
{
	int ret;
	struct ospfd *ospfd;

	ospfd = alloc_ospfd();

	ret = parse_options(&ospfd->opts, ac, av);
	if (ret != SUCCESS)
		err_msg_die(EXIT_FAILURE, "Can't parse command line");

	ret = init_network(ospfd);
	if (ret != SUCCESS)
		err_msg_die(EXIT_FAILURE, "Can't initialize network parts");


	sleep(5);



	fini_network(ospfd);
	free_ospfd(ospfd);

	return EXIT_SUCCESS;
}


/* vim: set tw=78 ts=4 sw=4 sts=4 ff=unix noet: */
