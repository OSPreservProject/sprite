/*
 * netInt.h --
 *
 *	This defines the types and constants for the networking software.
 *
 * Copyright 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _NETINT
#define _NETINT

#include "sprite.h"
#include "list.h"
#include "net.h"

typedef struct NetInterface {
    char	*name;
    int		 number;
    unsigned int ctrlAddr;
    Boolean	(*init)();
} NetInterface;
/*
 * A transmission queue element.
 */

typedef struct {
    List_Links		links;
    Net_EtherHdr	*etherHdrPtr;		/* Ethernet header with address
						 * of receiver already set */
    Net_ScatterGather	*scatterGatherPtr;	/* The scatter/gather array. */
    int			scatterGatherLength;	/* Number of items in the 
						   scatter/gather array. */
} NetXmitElement;

/*
 * The routines for the use of the proper ethernet controller.  We should
 * fix this to support multiple interfaces!
 */
typedef struct {
    Boolean (*init)();
    void (*output)();
    void (*intr)();
    void (*reset)();
} NetEtherFuncs;
extern	NetEtherFuncs	netEtherFuncs;

/*
 * Procedures for the internet packet handler.
 */
extern	void	NetEtherHandler();

extern	void	NetOutputWakeup();

#endif _NETINT
