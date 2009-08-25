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

#define	OSPF_DEFAULT_HELLO_INTERVAL 15u
#define	OSPF_DEFAULT_ROUTER_PRIORITY 1u
#define	OSPF_DEFAULT_ROUTER_DEAD_INTERVAL 40u

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
 * via the struct rd. This routing domain reflect the dynamic
 * view of routing. Configuration parameters is saved within
 * struct rc_rd */
struct rd {
	char if_name[IF_NAMESIZE];
	unsigned int if_index;
	unsigned int if_flags;
	struct list_e *ip_addr_list;
};

struct stats {
	long long packets_rx;
	long long ospf_rx;
	long long hello_rx;
	long long hello_tx;
};

struct network {
	int fd;
	struct stats stats;
	struct list_e *rd_list;
};

#define	MAX_LEN_DESCRIPTION 1024


/* Section 9. - The Interface Data Structure */
struct rc_rd {

	/* The name of the interface, e.g. eth0, ... */
	char if_name[IF_NAMESIZE];

	/* A short description of this interface (for
       more usefull error/debug or status messages if
       several interfaces are available */
	char description[MAX_LEN_DESCRIPTION];

	/* The Area ID of the area to which the attached network
       belongs. All routing protocol packets originating from
       the interface are labelled with this Area ID. */
	uint32_t area_id;

	/* The cost of sending a data packet on the interface,
       expressed in the link state metric. This is advertised
       as the link cost for this interface in the router-LSA.
       The cost of an interface must be greater than zero. */
	uint32_t costs;

	/* HelloInterval - The length of time, in seconds, between
       the Hello packets that the router sends on the interface.
       Advertised in Hello packets sent out this interface. */
	uint16_t hello_interval;

	/* IP interface address & IP interface mask:
	   The IP address associated with the interface.
       This appears as the IP source address in all routing
       protocol packets originated over this interface.
       Interfaces to unnumbered point-to-point networks do not
       have an associated IP address.
       Also referred to as the subnet mask, this indicates the portion
       of the IP interface address that identifies the attached network.
       Masking the IP interface address with the IP interface
       mask yields the IP network number of the attached network.  On
       point-to-point networks and virtual links, the IP interface mask
       is not defined. On these networks, the link itself is not
       assigned an IP network number, and so the addresses of each side
       of the link are assigned independently, if they are assigned at
       all. */
	struct ip_addr ip_addr;

	/* The number of seconds before the router's neighbors will declare
       it down, when they stop hearing the router's Hello Packets.
       Advertised in Hello packets sent out this interface. */
	uint16_t router_dead_interval;

	/* The estimated number of seconds it takes to transmit a Link
       State Update Packet over this interface.  LSAs contained in the
       Link State Update packet will have their age incremented by this
       amount before transmission.  This value should take into account
       transmission and propagation delays; it must be greater than
       zero. */
	uint16_t inf_trans_delay;
};

#define	EVENT_BACKING_STORE_HINT 64
#define	EVENT_ARRAY_SIZE 64
struct ev {
	int fd;
	size_t size;
};


struct ospfd {
	uint32_t router_id;
	struct opts opts;
	struct network network;
	struct ev ev;
	struct list_e *rc_rd_list;
};

/* cli_opt.c */
int parse_cli_options(struct ospfd *, int, char **);


#endif /* OSPFD */

/* vim:set ts=4 sw=4 sts=4 tw=78 ff=unix noet: */
