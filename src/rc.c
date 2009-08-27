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
#include "nbr.h"

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

static struct rc_rd *init_new_rc_rd(struct ospfd *ospfd, char *inf_name)
{
	struct rc_rd *rc_rd = alloc_rc_rd();

	/* insert into global rc_rd list */
	xospfd->rc_rd_list = list_insert_after(ospfd->rc_rd_list, rc_rd);

	/* save interface name */
	memcpy(rc_rd->if_name, inf_name,
			min((strlen(inf_name) + 1), sizeof(rc_rd->if_name)));

	return rc_rd;
}

static int search_rc_rd_for_interface(void *d1, void *d2)
{
	struct rc_rd *rc_rd = (struct rc_rd *) d1;
	char *if_name = (char *) d2;

	return !strcmp(rc_rd->if_name, if_name);
}

static struct rc_rd *get_rc_rd_by_interface(struct ospfd *ospfd, char *interface)
{
	struct rc_rd *rc_rd;
	struct list_e *list;

	list = list_search(ospfd->rc_rd_list, search_rc_rd_for_interface, interface);
	if (list == NULL) {
		rc_rd = init_new_rc_rd(xospfd, interface);
	} else {
		rc_rd = list->data;
	}

	return rc_rd;
}

void rc_set_area(char *interface, char *area)
{
	struct rc_rd *rc_rd = get_rc_rd_by_interface(xospfd, interface);

	/* and save the value */
	rc_rd->area_id = atoi(area);

	/* allocated via lexer - can now be freed */
	free(interface); free(area);
}

void rc_set_costs(char *interface, char *costs)
{
	struct rc_rd *rc_rd = get_rc_rd_by_interface(xospfd, interface);

	/* and save the value */
	rc_rd->costs = atoi(costs);

	/* 9. - The Interface Data Structure: The cost of an interface must
       be greater than zero. */
	if (rc_rd->costs < 1) {
		msg(xospfd, DEBUG, "configured link costs are to small - adjust to 1");
		rc_rd->costs = 1;
	}

	/* allocated via lexer - can be freed now */
	free(interface); free(costs);
}

void rc_set_ipv4_address(char *interface, char *ip, char *netmask)
{
	int ret;
	struct rc_rd *rc_rd = get_rc_rd_by_interface(xospfd, interface);

	rc_rd->ip_addr.family = AF_INET;
	ret = inet_pton(AF_INET, ip, &rc_rd->ip_addr.ipv4.addr);
	if (!ret) {
		err_sys("Cannot convert ip address (%s) into internal in6_addr");
		rc_rd->ip_addr.family = 0;
		return;
	}

	ret = inet_pton(AF_INET, netmask, &rc_rd->ip_addr.ipv4.netmask);
	if (!ret) {
		err_sys("Cannot convert ip netmask (%s) into internal in6_addr");
		rc_rd->ip_addr.family = 0;
		return;
	}

	/* allocated via lexer - can be freed now */
	free(interface); free(ip); free(netmask);
}

void rc_set_description(char *interface, char *description)
{
	struct rc_rd *rc_rd = get_rc_rd_by_interface(xospfd, interface);

	memcpy(rc_rd->description, description,
			min(strlen(description) + 1, sizeof(rc_rd->description)));

	/* allocated via lexer - can be freed now */
	free(interface); free(description);
}

void rc_set_hello_interval(char *interface, char *interval)
{
	struct rc_rd *rc_rd = get_rc_rd_by_interface(xospfd, interface);

	rc_rd->hello_interval = atoi(interval);

	free(interface); free(interval);
}

void rc_show_interface(char *interface)
{
	struct rc_rd *rc_rd;
	struct list_e *list;
	char addr[INET6_ADDRSTRLEN], mask[INET6_ADDRSTRLEN];

	list = list_search(xospfd->rc_rd_list, search_rc_rd_for_interface, interface);
	if (list == NULL) {
		fprintf(stderr, "Interface %s not konfigured!\n", interface);
		return;
	}

	rc_rd = list->data;

	switch (rc_rd->ip_addr.family) {
		case AF_INET:
			/* no error handling - this data structure is previously
			 * created via inet_pton() - there should be no problems */
			inet_ntop(AF_INET, &rc_rd->ip_addr.ipv4.addr, addr, INET6_ADDRSTRLEN);
			inet_ntop(AF_INET, &rc_rd->ip_addr.ipv4.netmask, mask, INET6_ADDRSTRLEN);
			break;
		case AF_INET6:
		default:
			err_msg_die(EXIT_FAILURE, "Programmed error - protocol not supported");
	}

	fprintf(stderr, "Interface: %s Area: %d Costs %d IP: %s Netmask %s\n",
			rc_rd->if_name, rc_rd->area_id, rc_rd->costs, addr, mask);

	free(interface);
}

void rc_set_id(char *id)
{
	xospfd->router_id = atoi(id);
	free(id);
}

void rc_set_interface_up(char *interface)
{
	struct rc_rd *rc_rd = get_rc_rd_by_interface(xospfd, interface);

	nbr_set_state(xospfd, rc_rd, INF_EV_INTERFACE_UP);

	free(interface);
}


/* vim: set tw=78 ts=4 sw=4 sts=4 ff=unix noet: */
