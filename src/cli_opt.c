#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <getopt.h>

#include "ospfd.h"
#include "shared.h"

static void set_default_options(struct opts *opts)
{
	opts->family        = AF_INET;
	opts->verbose_level = GENTLE;
	opts->rc_path       = NULL;
}

void free_options(struct ospfd *ospfd)
{
	struct opts *opts = &ospfd->opts;

	if (opts->rc_path)
		free(opts->rc_path);

	free(opts->me);
}

int parse_cli_options(struct ospfd *ospfd, int ac, char **av)
{
	int ret = SUCCESS, c;
	struct opts *opts = &ospfd->opts;

	set_default_options(opts);

	opts->me = xstrdup(av[0]);

	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{"verbose",          1, 0, 'v'},
			{"quite",            1, 0, 'q'},
			{"configuration",    1, 0, 'f'},
			{0, 0, 0, 0}
		};

		c = getopt_long(ac, av, "f:vq",
				long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'v':
			opts->verbose_level++;
			break;
		case 'q':
			opts->verbose_level = QUITSCENT;
			break;
		case 'f':
			opts->rc_path = xstrdup(optarg);
			break;
		case '?':
			break;

		default:
			err_msg("getopt returned character code 0%o ?", c);
			return FAILURE;
		}
	}

	return ret;
}



/* vim: set tw=78 ts=4 sw=4 sts=4 ff=unix noet: */
