#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ospfd.h"
#include "shared.h"
#include "rc.h"

extern FILE *yyin;

int open_rc_file(const struct ospfd *ospfd)
{
	if (ospfd->opts.rc_path) {
		yyin = fopen(ospfd->opts.rc_path, "r");
		if (yyin == NULL) {
			err_sys_die(EXIT_FAILURE, "cannot open configuration file (%s)",
					ospfd->opts.rc_path);
		}
		return SUCCESS;
	}
	/* ok, the user doesn't supplied any configuration
	 * file paths - we search in the standard environment */
	yyin = fopen("/etc/ospfd.conf", "r");
	if (yyin != NULL) {
		return SUCCESS;
	}

	yyin = fopen("/etc/ospfd/ospfd.conf", "r");
	if (yyin != NULL) {
		return SUCCESS;
	}

	return FAILURE;
}

/* see the comments within parse_rc_file() */
static struct ospfd *xospfd;

int parse_rc_file(struct ospfd *ospfd)
{
	int ret;

	ret = open_rc_file(ospfd);
	if (ret != SUCCESS) {
		err_msg("Cannot open a configuration file - try to start in passive mode");
		return SUCCESS;
	}

	/* ok, it is a little bit unclean, but anyway:
	 * It is not possible to pass user defined variables
	 * to yyparse(). Therefore within the parser process it is
	 * no possible to get a personal context. This programm,
	 * on the other hand needs a pointer to struct ospfd to store
	 * newly determined configuration. The solution is a clumsy hack
	 * to temporary save a pointer to ospfd -> xospfd even */
	xospfd = ospfd;

	/* and parse configuration file */
	yyparse();

	/* we set ospfd to NULL to detect any user that now does
	 * derefernce xospfd, which can by not valid in any further
	 * release */
	xospfd = NULL;

	return SUCCESS;
}

void rc_set_area(char *interface, char *area)
{
	fprintf(stderr, "interface: %s area: %s\n", interface, area);
}

void rc_set_metric(char *interface, char *metric)
{
	fprintf(stderr, "interface: %s metric: %s\n", interface, metric);
}

void rc_set_id(char *id)
{
	fprintf(stderr, "id: %s\n", id);
}


/* vim: set tw=78 ts=4 sw=4 sts=4 ff=unix noet: */
