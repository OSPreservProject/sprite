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

#include "list.h"
#include "net.h"

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
    void (*init)();
    void (*output)();
    void (*intr)();
    void (*reset)();
} NetEtherFuncs;
extern	NetEtherFuncs	netEtherFuncs;

/*
 * Procedures for the 3Com driver.
 */

extern	void	Net3CInit();
extern	void	Net3COutput();
extern	void	Net3CIntr();
extern	void	Net3CRestart();

/*
 * Procedures for the Intel driver.
 */

extern	void	NetIEInit();
extern	void	NetIEOutput();
extern	void	NetIEIntr();
extern	void	NetIERestart();

/*
 * Procedures for the internet packet handler.
 */
extern	void	NetEtherHandler();

extern	void	NetOutputWakeup();

#endif _NETINT
