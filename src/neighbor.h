#ifndef NEIGHBOR_H
#define NEIGHBOR_H

#include "ospfd.h"
#include "interface.h"

/* 10.1.  Neighbor states
   The state of a neighbor (really, the state of a conversation
   being held with a neighboring router) is documented in the
   following sections.  The states are listed in order of
   progressing functionality.  For example, the inoperative state
   is listed first, followed by a list of intermediate states
   before the final, fully functional state is achieved.  The
   specification makes use of this ordering by sometimes making
   references such as "those neighbors/adjacencies in state greater
   than X".  Figures 12 and 13 show the graph of neighbor state
   changes.  The arcs of the graphs are labelled with the event
   causing the state change.  The neighbor events are documented in
   Section 10.2. */

enum {
	/* Down - This is the initial state of a neighbor conversation.
	   It indicates that there has been no recent information received
       from the neighbor.  On NBMA networks, Hello packets may still
	   be sent to "Down" neighbors, although at a reduced frequency
	   (see Section 9.5.1). */
	NEIGHBOR_STATE_DOWN = 1,

	/* Attempt - This state is only valid for neighbors attached
	   to NBMA networks. It indicates that no recent information
	   has been received from the neighbor, but that a more
	   concerted effort should be made to contact the neighbor.
	   This is done by sending the neighbor Hello packets at intervals
	   of HelloInterval (see Section 9.5.1). */
	NEIGHBOR_STATE_ATTEMPT,

	/* Init - In this state, an Hello packet has recently been seen
	   from the neighbor. However, bidirectional communication has not
	   yet been established with the neighbor (i.e., the router itself
	   did not appear in the neighbor's Hello packet).  All neighbors
	   in this state (or higher) are listed in the Hello packets sent
	   from the associated interface. */
	NEIGHBOR_STATE_INIT,

	/* 2-Way - In this state, communication between the two routers is
       bidirectional.  This has been assured by the operation of the
	   Hello Protocol.  This is the most advanced state short of beginning
	   adjacency establishment.  The (Backup) Designated Router is selected
	   from the set of neighbors in state 2-Way or greater. */
	NEIGHBOR_STATE_TWO_WAY,

	/* ExStart - This is the first step in creating an adjacency between
	   the two neighboring routers.  The goal of this step is to decide
       which router is the master, and to decide upon the initial DD sequence
	   number. Neighbor conversations in this state or greater are called
	   adjacencies. */
	NEIGHBOR_STATE_EX_START,

	/* Exchange - In this state the router is describing its entire link
	   state database by sending Database Description packets to the neighbor.
	   Each Database Description Packet has a DD sequence number, and is
	   explicitly acknowledged.  Only one Database Description Packet is
	   allowed outstanding at any one time. In this state, Link State Request
	   Packets may also be sent asking for the neighbor's more recent LSAs.
	   All adjacencies in Exchange state or greater are used by the flooding
	   procedure.  In fact, these adjacencies are fully capable of transmitting
	   and receiving all types of OSPF routing protocol packets. */
	NEIGHBOR_STATE_EXCHANGE,

	/* Loading - In this state, Link State Request packets are sent to the
	   neighbor asking for the more recent LSAs that have been discovered
	   (but not yet received) in the Exchange state. */
	NEIGHBOR_STATE_LOADING,

	/* Full - In this state, the neighboring routers are fully adjacent.
       These adjacencies will now appear in router-LSAs and network-LSAs. */
	NEIGHBOR_STATE_FULL
};

/* TODO: document the following neighbor events (10.2.) */
enum {
	NEIGHBOR_EV_HELLO_RECEIVED = 1,
	NEIGHBOR_EV_START,
	NEIGHBOR_EV_TWO_WAY_RECEIVED,
	NEIGHBOR_EV_NEGOTIATION_DONE,
	NEIGHBOR_EV_EXCHANGE_DONE,
	NEIGHBOR_EV_BAD_LS_REQ,
	NEIGHBOR_EV_LOADING_DONE,
	NEIGHBOR_EV_ADJ_OK,
	NEIGHBOR_EV_SEQUENCE_NUMBER_MISMATCH,
	NEIGHBOR_EV_ONE_WAY,
	NEIGHBOR_EV_KILL_NBR,
	NEIGHBOR_EV_INACTIVITY_TIMER,
	NEIGHBOR_EV_LL_DOWN
};

struct neighbor *neighbor_by_id(struct ospfd *, struct interface_data *, uint32_t);
int neighbor_start_inactive_timer(struct ospfd *, struct neighbor *);
int neighbor_cancel_inactive_timer(struct ospfd *, struct neighbor *);
void interface_add_neighbor(struct interface_data *, struct neighbor *);
struct neighbor *alloc_neighbor(void);
int neighbor_process_event(struct ospfd *, struct neighbor *, int);

#endif /* NEIGHBOR_H */
