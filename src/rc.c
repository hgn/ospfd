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

static int search_rc_rd_for_interface(void *d1, void *d2)
{
	struct rc_rd *rc_rd = (struct rc_rd *) d1;
	char *if_name = (char *) d2;

	return !strcmp(rc_rd->if_name, if_name);
}

void rc_set_area(char *interface, char *area)
{
	struct rc_rd *rc_rd;
	struct list_e *list;

	list = list_search(xospfd->rc_rd_list, search_rc_rd_for_interface, interface);
	if (list == NULL) {
		rc_rd = xzalloc(sizeof(struct rc_rd));
		xospfd->rc_rd_list = list_insert_after(xospfd->rc_rd_list, rc_rd);
		memcpy(rc_rd->if_name, interface,
				min((strlen(interface) + 1), sizeof(rc_rd->if_name)));
	} else {
		rc_rd = list->data;
	}

	/* and save the value */
	rc_rd->area_id = atoi(area);

	/* allocated via lexer - can now be freed */
	free(interface); free(area);
}

void rc_set_metric(char *interface, char *metric)
{
	struct rc_rd *rc_rd;
	struct list_e *list;

	list = list_search(xospfd->rc_rd_list, search_rc_rd_for_interface, interface);
	if (list == NULL) {
		rc_rd = xzalloc(sizeof(struct rc_rd));
		xospfd->rc_rd_list = list_insert_after(xospfd->rc_rd_list, rc_rd);
		memcpy(rc_rd->if_name, interface,
				min((strlen(interface) + 1), sizeof(rc_rd->if_name)));
	} else {
		rc_rd = list->data;
	}

	/* and save the value */
	rc_rd->metric = atoi(metric);

	/* allocated via lexer - can now be freed */
	free(interface); free(metric);
}

void rc_show_interface(char *interface)
{
	struct rc_rd *rc_rd;
	struct list_e *list;

	list = list_search(xospfd->rc_rd_list, search_rc_rd_for_interface, interface);
	if (list == NULL) {
		fprintf(stderr, "Interface %s not konfigured!\n", interface);
		return;
	}

	rc_rd = list->data;

	fprintf(stderr, "Interface: %s Area: %d Metric %d\n",
			rc_rd->if_name, rc_rd->area_id, rc_rd->metric);

	free(interface);
}

void rc_set_id(char *id)
{
	fprintf(stderr, "id: %s\n", id);
}


/* vim: set tw=78 ts=4 sw=4 sts=4 ff=unix noet: */
