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

#include <sprite.h>
#include <list.h>
#include <bf.h>
#include <net.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * The following is a NIL function pointer.
 */

#define NILPROC	(void (*)()) NIL

/*
 * The following macros are used to access bit fields in strings of bytes.
 * The upper 24 bits of the index are the offset of the bit field, and
 * the lower 8 bits are the size of the bit field.  See bf.h for info
 * on the Bf macros.
 */

#define NetBfByteSet(ptr, index, value)			\
    Bf_ByteSet(ptr, (index) >> 8, (index) & 0xff, value)

#define NetBfByteTest(ptr, index, value)			\
    Bf_ByteTest(ptr, (index) >> 8, (index) & 0xff, value)

#define NetBfByteGet(ptr, index)			\
    Bf_ByteGet(ptr, (index) >> 8, (index) & 0xff)

#define NetBfShortSet(ptr, index, value)			\
    Bf_HalfwordSet(ptr, (index) >> 8, (index) & 0xff, value)

#define NetBfShortTest(ptr, index, value)			\
    Bf_HalfwordTest(ptr, (index) >> 8, (index) & 0xff, value)

#define NetBfShortGet(ptr, index)			\
    Bf_HalfwordGet(ptr, (index) >> 8, (index) & 0xff)

#define NetBfWordSet(ptr, index, value)			\
    Bf_WordSet(ptr, (index) >> 8, (index) & 0xff, value)

#define NetBfWordTest(ptr, index, value)			\
    Bf_WordTest(ptr, (index) >> 8, (index) & 0xff, value)

#define NetBfWordGet(ptr, index)			\
    Bf_WordGet(ptr, (index) >> 8, (index) & 0xff)

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

extern	Net_EtherStats	net_EtherStats;
extern	Net_Address 	netEtherBroadcastAddress;
extern Net_Interface	*netInterfaces[];
extern int		netNumInterfaces;
extern Net_Address	netZeroAddress;
extern Boolean		netDebug;
/*
 * Procedures for the internet packet handler.
 */
extern void NetOutputWakeup _ARGS_((Sync_Semaphore *mutexPtr));

/*
 * Forward declarations.
 */

extern Net_Route *NetAllocRoute _ARGS_((void));


extern void NetEtherInit _ARGS_((void));

#endif /* _NETINT */
