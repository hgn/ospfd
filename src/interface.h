#ifndef INTERFACE_H
#define INTERFACE_H

#include "ospfd.h"

#define	NEIGHBOR_INACTIVE_TIMER_DISABLED INT32_MIN

struct inactivity_timer_data {
	struct ospfd *ospfd;
	struct neighbor *neighbor;
};

struct neighbor {

	/* State - the functional level of the neighbor
	   conversation. This is described in more detail
	   in Section 10.1. */
	int state;

    /* Inactive Timer - a single shot timer whose firing
	   indicates that no Hello Packet has been seen from
	   this neighbor recently. The length of the timer is
	   RouterDeadInterval seconds. */
	int inactive_timer;
	struct inactivity_timer_data *inactivity_timer_data;
	struct ev_data *timer_priv_data;

	/* Master/Slave - When the two neighbors are exchanging
	   databases, they form a master/slave relationship.
	   The master sends the first Database Description Packet,
	   and is the only part that is allowed to retransmit.
	   The slave can only respond to the master's Database
       Description Packets. The master/slave relationship is
       negotiated in state ExStart. */
	int master_slave_relationship;

    /* DD Sequence Number -- the DD Sequence number of the
	   Database Description packet that is currently being
	   sent to the neighbor. */
	int dd_sequence_number;

    /* Last received Database Description packet -- The initialize(I),
	   more (M) and master(MS) bits, Options field, and DD sequence number
	   contained in the last Database Description packet received from
	   the neighbor. Used to determine whether the next Database
	   Description packet received from the neighbor is a duplicate. */
	int last_received_dd_packet;

	/* Neighbor ID -- The OSPF Router ID of the neighboring router.
	   The Neighbor ID is learned when Hello packets are received from
	   the neighbor, or is configured if this is a virtual adjacency
	   (see Section C.4). */
	uint32_t neighbor_id;

	/* Neighbor Priority -- the Router Priority of the neighboring router.
	   Contained in the neighbor's Hello packets, this item is used when
	   selecting the Designated Router for the attached network. */
	int neighbor_priority;

    /* Neighbor IP address -- The IP address of the neighboring router's
	   interface to the attached network. Used as the Destination IP address
	   when protocol packets are sent as unicasts along this adjacency.
       Also used in router-LSAs as the Link ID for the attached network
       if the neighboring router is selected to be Designated Router
       (see Section 12.4.1).  The Neighbor IP address is learned when
       Hello packets are received from the neighbor. For virtual links,
	   the Neighbor IP address is learned during the routing table build
	   process (see Section 15). */
	int neighbor_ip_address;

	/* Neighbor Options -- the optional OSPF capabilities supported by
	   the neighbor. Learned during the Database Exchange process
	   (see Section 10.6). The neighbor's optional OSPF capabilities are
	   also listed in its Hello packets. This enables received Hello Packets
	   to be rejected (i.e., neighbor relationships will not even start to
       form) if there is a mismatch in certain crucial OSPF capabilities
	   (see Section 10.5). The optional OSPF capabilities are documented in
	   Section 4.5. */
	int neighbor_options;
};

void interface_set_state(struct ospfd *ospfd, struct interface_data *interface_data, int new_state);
struct interface_data *alloc_interface_data(void);
int interface_data_name_cmp(void *, void *);
struct interface_data *interface_data_for_index(const struct ospfd *, unsigned int);

#endif /* INTERFACE_H */
