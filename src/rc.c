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
#include "interface.h"

extern FILE *yyin;
int yyparse(void);

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
	ret = yyparse();
	if (ret == 1)
		return FAILURE;

	/* we set ospfd to NULL to detect any user that now does
	 * derefernce xospfd, which can by not valid in any further
	 * release */
	xospfd = NULL;

	return SUCCESS;
}

static struct interface_data *init_new_interface_data(struct ospfd *ospfd, char *inf_name)
{
	struct interface_data *interface_data = alloc_interface_data();

	/* insert into global interface_data list */
	xospfd->interface_data_list = list_insert_after(ospfd->interface_data_list, interface_data);

	/* save interface name */
	memcpy(interface_data->if_name, inf_name,
			min((strlen(inf_name) + 1), sizeof(interface_data->if_name)));

	return interface_data;
}

static int search_interface_data_for_interface(void *d1, void *d2)
{
	struct interface_data *interface_data = (struct interface_data *) d1;
	char *if_name = (char *) d2;

	return !strcmp(interface_data->if_name, if_name);
}

static struct interface_data *get_interface_data_by_interface(struct ospfd *ospfd, char *interface)
{
	struct interface_data *interface_data;
	struct list_e *list;

	list = list_search(ospfd->interface_data_list, search_interface_data_for_interface, interface);
	if (list == NULL) {
		interface_data = init_new_interface_data(xospfd, interface);
	} else {
		interface_data = list->data;
	}

	return interface_data;
}

void rc_set_area(char *interface, char *area)
{
	struct interface_data *interface_data = get_interface_data_by_interface(xospfd, interface);

	/* and save the value */
	interface_data->area_id = atoi(area);

	/* allocated via lexer - can now be freed */
	free(interface); free(area);
}

void rc_set_costs(char *interface, char *costs)
{
	struct interface_data *interface_data = get_interface_data_by_interface(xospfd, interface);

	/* and save the value */
	interface_data->costs = atoi(costs);

	/* 9. - The Interface Data Structure: The cost of an interface must
       be greater than zero. */
	if (interface_data->costs < 1) {
		msg(xospfd, DEBUG, "configured link costs are to small - adjust to 1");
		interface_data->costs = 1;
	}

	/* allocated via lexer - can be freed now */
	free(interface); free(costs);
}

void rc_set_ipv4_address(char *interface, char *ip, char *netmask)
{
	int ret;
	struct interface_data *interface_data = get_interface_data_by_interface(xospfd, interface);

	interface_data->ip_addr.family = AF_INET;
	ret = inet_pton(AF_INET, ip, &interface_data->ip_addr.ipv4.addr);
	if (!ret) {
		err_sys("Cannot convert ip address (%s) into internal in6_addr");
		interface_data->ip_addr.family = 0;
		return;
	}

	ret = inet_pton(AF_INET, netmask, &interface_data->ip_addr.ipv4.netmask);
	if (!ret) {
		err_sys("Cannot convert ip netmask (%s) into internal in6_addr");
		interface_data->ip_addr.family = 0;
		return;
	}

	/* allocated via lexer - can be freed now */
	free(interface); free(ip); free(netmask);
}

void rc_set_description(char *interface, char *description)
{
	struct interface_data *interface_data = get_interface_data_by_interface(xospfd, interface);

	memcpy(interface_data->description, description,
			min(strlen(description) + 1, sizeof(interface_data->description)));

	/* allocated via lexer - can be freed now */
	free(interface); free(description);
}

void rc_set_hello_interval(char *interface, char *interval)
{
	struct interface_data *interface_data = get_interface_data_by_interface(xospfd, interface);

	interface_data->hello_interval = atoi(interval);

	free(interface); free(interval);
}

void rc_show_interface(char *interface)
{
	struct interface_data *interface_data;
	struct list_e *list;
	char addr[INET6_ADDRSTRLEN], mask[INET6_ADDRSTRLEN];

	list = list_search(xospfd->interface_data_list, search_interface_data_for_interface, interface);
	if (list == NULL) {
		fprintf(stderr, "Interface %s not konfigured!\n", interface);
		return;
	}

	interface_data = list->data;

	switch (interface_data->ip_addr.family) {
		case AF_INET:
			/* no error handling - this data structure is previously
			 * created via inet_pton() - there should be no problems */
			inet_ntop(AF_INET, &interface_data->ip_addr.ipv4.addr, addr, INET6_ADDRSTRLEN);
			inet_ntop(AF_INET, &interface_data->ip_addr.ipv4.netmask, mask, INET6_ADDRSTRLEN);
			break;
		case AF_INET6:
		default:
			err_msg_die(EXIT_FAILURE, "Programmed error - protocol not supported");
	}

	fprintf(stderr, "Interface: %s Area: %d Costs %d IP: %s Netmask %s\n",
			interface_data->if_name, interface_data->area_id, interface_data->costs, addr, mask);

	free(interface);
}

void rc_set_id(char *id)
{
	xospfd->router_id = atoi(id);
	free(id);
}

void rc_set_interface_up(char *interface)
{
	struct interface_data *interface_data = get_interface_data_by_interface(xospfd, interface);

	interface_set_state(xospfd, interface_data, INF_EV_INTERFACE_UP);

	free(interface);
}

void rc_set_router_priority(char *interface, char *router_priority)
{
	int val;
	struct interface_data *interface_data = get_interface_data_by_interface(xospfd, interface);

	val = xatoi(router_priority);
	if (val < 0) {
		msg(xospfd, GENTLE, "router priority negativ: %d - must be positiv!"
				" The programm falls back to the default %u !",
				OSPF_DEFAULT_ROUTER_PRIORITY);
		val = OSPF_DEFAULT_ROUTER_PRIORITY;
	}

	interface_data->router_priority = val;

	free(interface); free(router_priority);
}

void rc_set_router_dead_interval(char *interface, char *router_dead_interval)
{
	int val;
	struct interface_data *interface_data = get_interface_data_by_interface(xospfd, interface);

	val = xatoi(router_dead_interval);
	if (val < 0) {
		msg(xospfd, GENTLE, "router dead interval negativ: %u - must be positiv!"
				" The programm falls back to the default value (%d sec)!",
				OSPF_DEFAULT_ROUTER_DEAD_INTERVAL);
		val = OSPF_DEFAULT_ROUTER_DEAD_INTERVAL;
	}

	interface_data->router_dead_interval = val;

	free(interface); free(router_dead_interval);
}

void rc_set_type(char *interface, char *type)
{
	int val = INF_TYPE_UNKNOWN;;
	struct interface_data *interface_data = get_interface_data_by_interface(xospfd, interface);

	if (!strcmp(type, INF_TYPE_BROADCAST_STR)) {
		val = INF_TYPE_BROADCAST;
	} else if (!strcmp(type, INF_TYPE_PTP_STR)) {
		val = INF_TYPE_PTP;
	} else if (!strcmp(type, INF_TYPE_PTMP_STR)) {
		val = INF_TYPE_PTMP;
	} else if (!strcmp(type, INF_TYPE_NBMA_STR)) {
		val = INF_TYPE_NBMA;
	} else if (!strcmp(type, INF_TYPE_VIRTUAL_LINK_STR)) {
		val = INF_TYPE_VIRTUAL_LINK;
	} else {
		msg(xospfd, GENTLE, "router interface type (%s) not supportet - Fall back to broadcast!",
				type);
		val = INF_TYPE_BROADCAST;
	}

	interface_data->type = val;

	free(interface); free(type);
}

/* vim: set tw=78 ts=4 sw=4 sts=4 ff=unix noet: */
