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

int parse_rc_file(struct ospfd *ospfd)
{
	int ret;

	ret = open_rc_file(ospfd);
	if (ret != SUCCESS) {
		err_msg("Cannot open a configuration file - try to start in passive mode");
		return SUCCESS;
	}

	/* and parse configuration file */
	yyparse();

	return SUCCESS;
}

void rc_add_area(char *area)
{
	fprintf(stderr, "area: %s\n", area);
}

void rc_add_id(char *id)
{
	fprintf(stderr, "id: %s\n", id);
}


/* vim: set tw=78 ts=4 sw=4 sts=4 ff=unix noet: */
