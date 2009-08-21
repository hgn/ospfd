#ifndef OSPFD_H
#define	OSPFD_H

#include <inttypes.h>
#include <netdb.h>
#include <sys/socket.h>
#include <net/if.h>

#define min(x,y) ({                     \
        typeof(x) _x = (x);             \
        typeof(y) _y = (y);             \
        (void) (&_x == &_y);    \
        _x < _y ? _x : _y; })

#define max(x,y) ({                     \
        typeof(x) _x = (x);             \
        typeof(y) _y = (y);             \
        (void) (&_x == &_y);    \
        _x > _y ? _x : _y; })

#if !defined likely && !defined unlikely
# define likely(x)   __builtin_expect(!!(x), 1)
# define unlikely(x) __builtin_expect(!!(x), 0)
#endif

#define	PROGRAMNAME   "ospfd"
#define	VERSIONSTRING "0.0.1"

/* function return codes */
#define SUCCESS 0
#define FAILURE -1

/* program error codes */
#define	EXIT_NETWORK 2

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define	OSPF_DEFAULT_HELLO_INTERVAL 1u

enum {
	QUITSCENT = 0,
	GENTLE,
	VERBOSE,
	DEBUG
};

struct opts {
	/* daemon common values */
	char *me;
	int verbose_level;
	char *rc_path;

	/* ospf related */
	char *area;
	sa_family_t family;
};

#include <sys/types.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <netdb.h>

#include <netinet/in.h> /* sockaddr_in & sockaddr_in6 */

#include "clist.h"

struct ip_addr {
	sa_family_t family;
	union {
		struct {
			struct in6_addr addr;
			struct in6_addr netmask;
			uint32_t scope;
		} addr6;
		struct {
			struct in_addr addr;
			struct in_addr netmask;
			struct in_addr broadcast;
		} addr4;
	} u;
#define	ipv4 u.addr4
#define	ipv6 u.addr6
};

/* the Routing Domains, available at this
 * Router. Normaly these are all active network
 * interface activated for routing (e.g. eth0, eth1),
 * but in principle nearly all interface can be *gates*
 * to another area/whatever so all interfaces are available
 * via the struct rd */
struct rd {
	char if_name[IF_NAMESIZE];
	unsigned int if_index;
	unsigned int if_flags;
	struct list_e *ip_addr_list;
};

struct network {
	int fd;
	struct list_e *rd_list;
};

#define	EVENT_BACKING_STORE_HINT 64
#define	EVENT_ARRAY_SIZE 64
struct ev {
	int fd;
	size_t size;
};


struct ospfd {
	struct opts opts;
	struct network network;
	struct ev ev;
};

/* cli_opt.c */
int parse_cli_options(struct ospfd *, int, char **);


#endif /* OSPFD */

/* vim:set ts=4 sw=4 sts=4 tw=78 ff=unix noet: */
