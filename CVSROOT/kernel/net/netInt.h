/*
 * netInt.h --
 *
 *	This defines the types and constants for the networking software.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
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
