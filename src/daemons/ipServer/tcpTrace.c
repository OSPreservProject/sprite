/*
 * tcpTrace.c --
 *
 *	Routines to log traces of major events that occur for a TCP socket.
 *
 *	Based on 4.3BSD	@(#)tcp_debug.c	7.1 (Berkeley) 6/5/86
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/daemons/ipServer/RCS/tcpTrace.c,v 1.3 89/03/23 09:57:03 brent Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "netInet.h"
#include "ipServer.h"
#include "tcp.h"
#include "tcpInt.h"
#include "tcpTimer.h"

/*
 * A TraceRecord contains a snapshot of important information.
 */
typedef  struct	{
    int			time;		/* Time when the trace routine was 
					 * called. */
    TCPTraceCmd		command;	/* The type of tracing desired. */
    int			prevState;	/* Previous state of the TCP control 
					 * block. */
    Net_TCPHeader	header;		/* TCP header to be logged. */
    TCPControlBlock	controlBlock;	/* Contents of the control block. */
} TraceRecord;

static char    *traceNames[] = {
    "Input", "Output", "Respond", "Drop"
};

static char *flagNames[] = { "FIN", "SYN", "RESET", "PUSH", "ACK", "URG",};

char *tcbStateNames[] = {
    "CLOSED",
    "LISTEN",
    "SYN_SENT",
    "SYN_RECEIVED",
    "ESTABLISHED",
    "CLOSE_WAIT",
    "LAST_ACK",
    "FIN_WAIT_1",
    "FIN_WAIT_2",
    "CLOSING",
    "TIME_WAIT",
};

#define NUM_TRACES 100
static TraceRecord traceArray[NUM_TRACES];
static int traceNum = 0;



/*
 *----------------------------------------------------------------------
 *
 * TCPTrace --
 *
 *	Used to trace important changes to a TCP control block.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The traceArray is updated.
 *
 *----------------------------------------------------------------------
 */

void
TCPTrace(command, prevState, tcbPtr, headerPtr, dataLen)
    TCPTraceCmd		command;	/* Type of trace. */
    int			prevState;	/* Previous TCB state. */
    TCPControlBlock	*tcbPtr;	/* TCB to be saved. */
    Net_TCPHeader	*headerPtr;	/* TCP header to be saved. */
    int			dataLen;	/* Amount of data in the packet. */
{
    register TraceRecord *tracePtr;
    TCPSeqNum	seq;
    TCPSeqNum	ack;

    tracePtr = &traceArray[traceNum];
    traceNum += 1;
    if (traceNum == NUM_TRACES) {
	traceNum = 0;
    }

    tracePtr->time	= IPS_GetTimestamp();
    tracePtr->command	= command;
    tracePtr->prevState	= prevState;

    if (tcbPtr != NULL) {
	tracePtr->controlBlock = *tcbPtr;
    } else {
	bzero((Address)&tracePtr->controlBlock, sizeof(*tcbPtr));
    }
    if (headerPtr != NULL) {
	tracePtr->header = *headerPtr;
    } else {
	bzero((Address)&tracePtr->header, sizeof(*headerPtr));
    }

    if (!ips_Debug) {
	return;
    }

    (void) printf("%s: ", traceNames[(int)command]);

    if (tcbPtr != NULL) {
	(void) printf("%x %s:", tcbPtr, tcbStateNames[(int)prevState]);
    } else {
	(void) printf("???????? ");
    }

    switch (command) {

	case TCP_TRACE_INPUT:
	case TCP_TRACE_OUTPUT:
	case TCP_TRACE_DROP:
	    if (headerPtr == NULL) {
		break;
	    }
	    seq = headerPtr->seqNum;
	    ack = headerPtr->ackNum;
	    if (command == TCP_TRACE_OUTPUT) {
		seq = Net_NetToHostInt(seq);
		ack = Net_NetToHostInt(ack);
	    }
	    if (dataLen != 0) {
		(void) printf("[%x..%x)", seq, seq+dataLen);
	    } else {
		(void) printf("%x", seq);
	    }
	    (void) printf("@%x, urgent=%x", ack, headerPtr->urgentOffset);
	    if (headerPtr->flags!= 0) {
		TCPPrintHdrFlags(stdout, headerPtr->flags);
	    }
	    break;
    }

    /* 
     * Print out internal state of *tcbPtr.
     */

    if (tcbPtr != NULL) {
	(void) printf(" -> %s\n", tcbStateNames[(int)tcbPtr->state]);

	(void) printf("\trecv.(next,window,urgPtr) (%x,%x,%x)\n",
		tcbPtr->recv.next, tcbPtr->recv.window, tcbPtr->recv.urgentPtr);
	(void) printf("\tsend.(unAck,next,maxSent) (%x,%x,%x)\n",
		tcbPtr->send.unAck, tcbPtr->send.next, tcbPtr->send.maxSent);
	(void) printf("\tsend.(updateSeq#,updateAck#,window) (%x,%x,%x)\n",
		tcbPtr->send.updateSeqNum, tcbPtr->send.updateAckNum, 
		tcbPtr->send.window);
    }
    (void) printf("\n");
}


/*
 *----------------------------------------------------------------------
 *
 * TCPPrintHdrFlags --
 *
 *	Prints the state of the flags in the TCP header.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TCPPrintHdrFlags(stream, flags)
    FILE		*stream;	/* Where to print. */
    unsigned short	flags;		/* TCP packet header flags. */
{
    register int i;

    char *cp = "<";
    for (i=0; i < 6; i++) {
	if ((1 << i) & flags) {
	    (void) fprintf(stream, "%s%s", cp, flagNames[i]);
	    cp = ", ";
	}
    }
    (void) fprintf(stream, ">");
}


/*
 *----------------------------------------------------------------------
 *
 * TCP_PrintInfo --
 *
 *	A routine to print out the state of a TCB.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TCP_PrintInfo(data)
    ClientData	data;		/* Really a pointer to a TCP. */
{
    TCPControlBlock *tcbPtr = (TCPControlBlock *) data;

    if (tcbPtr != (TCPControlBlock *) NULL) {
	(void) fprintf(stderr, 
		"%10s flags=%x unAck=%d s.next=%d r.next=%d\n",
		tcbStateNames[(int)tcbPtr->state],
		tcbPtr->flags,
		tcbPtr->send.unAck,
		tcbPtr->send.next,
		tcbPtr->recv.next);
    }
}
