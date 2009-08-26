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

/* 9.1.  Interface states - The various states that router interfaces
   may attain is documented in this section.  The states are listed in order of
   progressing functionality.  For example, the inoperative state
   is listed first, followed by a list of intermediate states
   before the final, fully functional state is achieved. */

enum {
	/* Down - in this is the initial interface state.  In this state, the
	   lower-level protocols have indicated that the interface is
	   unusable.  No protocol traffic at all will be sent or
	   received on such a interface.  In this state, interface
	   parameters should be set to their initial values.  All
	   interface timers should be disabled, and there should be no
	   adjacencies associated with the interface. */
	INF_STATE_DOWN = 1,

	/* Loopback - in this state, the router's interface to the network is
       looped back. The interface may be looped back in hardware
	   or software. The interface will be unavailable for regular
	   data traffic. However, it may still be desirable to gain
	   information on the quality of this interface, either through
	   sending ICMP pings to the interface or through something
	   like a bit error test.  For this reason, IP packets may
	   still be addressed to an interface in Loopback state. To
       facilitate this, such interfaces are advertised in router-
       LSAs as single host routes, whose destination is the IP
       interface address. */
	INF_STATE_LOOPBACK,

	/* Waiting - in this state, the router is trying to determine the
       identity of the (Backup) Designated Router for the network.
	   To do this, the router monitors the Hello Packets it
	   receives.  The router is not allowed to elect a Backup
	   Designated Router nor a Designated Router until it
	   transitions out of Waiting state.  This prevents unnecessary
	   changes of (Backup) Designated Router. */
	INF_STATE_WAITING,

	/* Point-to-point - in this state, the interface is operational,
       and connects either to a physical point-to-point network or
       to a virtual link. Upon entering this state, the router attempts
       to form an adjacency with the neighboring router. Hello Packets
       are sent to the neighbor every HelloInterval seconds. */
	INF_STATE_POINT_TO_POINT,

	/* DR Other - The interface is to a broadcast or NBMA network on which
       another router has been selected to be the Designated
	   Router.  In this state, the router itself has not been
	   selected Backup Designated Router either.  The router forms
	   adjacencies to both the Designated Router and the Backup
	   Designated Router (if they exist). */
	INF_STATE_DR_OTHER,

    /* Backup - in this state, the router itself is the Backup Designated
       Router on the attached network.  It will be promoted to
	   Designated Router when the present Designated Router fails.
	   The router establishes adjacencies to all other routers
	   attached to the network.  The Backup Designated Router
	   performs slightly different functions during the Flooding
	   Procedure, as compared to the Designated Router (see Section
	   13.3).  See Section 7.4 for more details on the functions
	   performed by the Backup Designated Router. */
	INF_STATE_BACKUP,

	/* DR - in this state, this router itself is the Designated Router
       on the attached network.  Adjacencies are established to all
	   other routers attached to the network.  The router must also
	   originate a network-LSA for the network node.  The network-
	   LSA will contain links to all routers (including the
	   Designated Router itself) attached to the network.  See
	   Section 7.3 for more details on the functions performed by
	   the Designated Router. */
	INF_STATE_DR
};


/* Section 9. - The Interface Data Structure */
struct rc_rd {

	/* The name of the interface, e.g. eth0, ... */
	char if_name[IF_NAMESIZE];

	/* A short description of this interface (for
       more usefull error/debug or status messages if
       several interfaces are available */
	char description[MAX_LEN_DESCRIPTION];

	/* State - The functional level of an interface. State
	   determines whether or not full adjacencies are allowed
	   to form over the interface. State is also reflected in
	   the router's LSAs. */
	unsigned int state;

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
