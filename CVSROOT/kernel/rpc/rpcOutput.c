/* 
 * rpcOutput.c --
 *
 *	The output routine for the RPC system.  Large packets are
 *	fragmented upon ouput here.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "mach.h"
#include "rpc.h"
#include "rpcInt.h"
#include "rpcTrace.h"
#include "net.h"
#include "sync.h"
#include "status.h"
#include "dbg.h"
#include "mach.h"

/*
 * A delay variable that represents our preferred inter-fragment delay.
 * This is told to other machines by including it in the RPC header.
 * They use it to set the delay between packets in a fragmented send.
 */
short rpcMyDelay;
short rpcOutputRate;
short rpcDelay[NET_NUM_SPRITE_HOSTS];

short rpcLastDelay;

#ifdef PRINT_PACKETS
Boolean rpcDumpPackets = FALSE;
#endif PRINT_PACKETS

/*
 *----------------------------------------------------------------------
 *
 * RpcOutput --
 *
 *      Send a message to the host specified by spriteID.  Packets over a
 *      certain size are fragmented by this routine into several datagrams
 *      and reassembled by the dispatch routine of the receiver.  Our
 *      caller has to pass in an array of buffer specifications that this
 *      routine uses if it has to fragment the message.  The RPC header
 *      and the dontSendMask are also used in conjuction with fragmented
 *      packets.
 *      Master Lock note: This routine may busy wait for a few hundred
 *      microseconds in between sending successive packets of a large
 *      message.  This delay is defeated if there is a Master Lock held.
 *      *mutexPtr, if non-NIL, is released during this delay, and then
 *      reaquired.  (This is done because the DMA ethernet driver needs an
 *      interrupt serviced after each packet output.  Things work without
 *      enabling interrupts, but the packets get queued for output until
 *      after the caller of RpcOutput releases the MasterLock.  This
 *      causes one big delay between the 1rst and 2nd packet, and no delay
 *      between ones after that.)
 *
 * Results:
 *	SUCCESS on non-fragmented sends, or an error code related
 *	to fragmenting.
 *
 * Side effects:
 *	Call the ethernet output routine and trace.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
RpcOutput(spriteID, rpcHdrPtr, message, fragment, dontSendMask, mutexPtr)
    int			spriteID;	/* Destination host */
    register RpcHdr	*rpcHdrPtr;	/* The RPC header. */
    RpcBufferSet	*message;	/* A set of 3 scatter/gather vector
					 * elements that specify where the
					 * header, parameters, and data are. */
    RpcBufferSet	*fragment;	/* An array of buffer sets used if
					 * the message needs to be
					 * fragmented.  This needs to be
					 * previously set up so that the
					 * first buffer references valid
					 * storage for an RPC header. */
    unsigned int	dontSendMask;	/* For fragmenting.  Bits set in this
					 * mask suppress sending of the
					 * corresponding fragment.  A good
					 * value for this is returned in
					 * partial acknowlegment messages */
    int			*mutexPtr;	/* This mutex is released while we
					 * output the message.  If NIL, no
					 * mutex is released.  This is done
					 * in honor of DMA interfaces that
					 * interrupt after they output a packet.
					 */
{
#ifdef PRINT_PACKETS
    if (rpcDumpPackets) {
	register unsigned short *shortPtr;
	register int i;

	Sys_Printf("RpcOutput - Raw packet at %x:\n", rpcHdrPtr);
	shortPtr = (unsigned short *)rpcHdrPtr;
	for (i=0 ; i<sizeof(RpcHdr)/2 ; i++) {
	    Sys_Printf("%x ", *shortPtr);
	    shortPtr++;
	    if (((i+1)%8) == 0) {
		Sys_Printf("\n");
	    }
	}
	Sys_Printf("\n");

	Sys_Printf("RpcOutput - buffer set:\n");
	Sys_Printf("%x %x\n", message->rpcHdrBuffer.bufAddr,
		       message->rpcHdrBuffer.length);
	Sys_Printf("%x %x\n", message->paramBuffer.bufAddr,
		       message->paramBuffer.length);
	Sys_Printf("%x %x\n", message->dataBuffer.bufAddr,
		       message->dataBuffer.length);
    }
#endif PRINT_PACKETS
    /*
     * Mark our packets if we arn't up all the way.  This means that the
     * other host won't pay attention to our packets in terms of recovery.
     */
    if (!rpcServiceEnabled) {
	rpcHdrPtr->flags |= RPC_NOT_ACTIVE;
    }
    /*
     * Check to see if we have to fragment.   Note that we pack the first
     * fragment as full as possible so that it might contain more than
     * 1K of data.  If we fragment, however, we break things on 1K boundaries.
     */
    if (rpcHdrPtr->paramSize + rpcHdrPtr->dataSize >
	RPC_MAX_PACKET_SIZE - sizeof(Net_EtherHdr) - sizeof(RpcHdr)) {
	if (rpcHdrPtr->paramSize > RPC_MAX_PARAMSIZE) {
	    return(RPC_PARAMS_TOOBIG);
	} else if (rpcHdrPtr->dataSize > RPC_MAX_DATASIZE) {
	    return(RPC_DATA_TOOBIG);
	} else {
	    /*
	     * Send a fragmented message.
	     */
	    register int dataSize, paramSize;	/* Size remaining in data and
						 * parameter areas that needs
						 * to go in subsequent frags. */
	    int dataOffset, paramOffset;	/* Offset of fragment data */
	    int frag;				/* Fragment index */
	    int nfrags;				/* Total number of fragments */
	    int delay;				/* Inter-fragment delay */
	    register int fragID;		/* Bitmask ID of fragment.  The
						 * I'th bit of this is set on
						 * the I'th fragment. */
	    register int sendFragMask;		/* Complement of the dont-send
						 * mask. */
	    register RpcHdr *fragRpcHdrPtr;	/* RPC header of a fragment */

	    dataSize = rpcHdrPtr->dataSize;
	    paramSize = rpcHdrPtr->paramSize;

	    /*
	     * Loop one time to compute and record the sizes of the
	     * parameter and data areas of each fragment.
	     */
	    nfrags = 0;
	    dataOffset = 0;
	    paramOffset = 0;
	    while (dataSize > 0 || paramSize > 0) {
		register int dlen, plen;

		/*
		 * Fill the fragments with the parameter area followed by
		 * the data area.  The max size ethernet packet is used
		 * for best throughput.
		 */
		if (paramSize) {
		    plen = (RPC_MAX_PACKET_SIZE - sizeof(Net_EtherHdr)
						- sizeof(RpcHdr));
		    plen = (plen > paramSize ? paramSize : plen);
		    paramSize -= plen;
		} else {
		    plen = 0;
		}
		if (dataSize) {
		    dlen = (RPC_MAX_PACKET_SIZE - sizeof(Net_EtherHdr)
						- sizeof(RpcHdr) - plen);
		    dlen = (dlen > dataSize ? dataSize : dlen);
		    dataSize -= dlen;
		} else {
		    dlen = 0;
		}
		fragRpcHdrPtr = (RpcHdr *)fragment[nfrags].rpcHdrBuffer.bufAddr;

		fragRpcHdrPtr->paramSize = plen;
		fragRpcHdrPtr->dataSize = dlen;
		fragRpcHdrPtr->paramOffset = paramOffset;
		fragRpcHdrPtr->dataOffset = dataOffset;

		fragment[nfrags].paramBuffer.length = plen;
		fragment[nfrags].paramBuffer.bufAddr =
			message->paramBuffer.bufAddr + paramOffset;

		fragment[nfrags].dataBuffer.length = dlen;
		fragment[nfrags].dataBuffer.bufAddr = 
			message->dataBuffer.bufAddr + dataOffset;

		dataOffset += dlen;
		paramOffset += plen;
		nfrags++;
	    }

	    /*
	     * An inter-fragment delay is setup when going to a slower machine.
	     */
	    if (rpcDelay[spriteID] > rpcOutputRate) {
		delay = rpcDelay[spriteID] - rpcOutputRate;
	    } else {
		delay = 0;
	    }
	    /*
	     * Loop (again) to output the fragments.  Fragments are not output
	     * if their bit is set in the dontSendMask.  If dontSentMask
	     * would suppress all fragments we just send the last one
	     * as a keep-alive.
	     */
	    if (dontSendMask == rpcCompleteMask[nfrags]) {
		dontSendMask &= ~(1 << (nfrags - 1));
	    }
	    sendFragMask = ~dontSendMask;
	    for (fragID = 1, frag = 0;
		 frag < nfrags;
		 fragID <<= 1, frag++) {
		if (fragID & sendFragMask) {

		    fragRpcHdrPtr =
			    (RpcHdr *)fragment[frag].rpcHdrBuffer.bufAddr;

		    sendFragMask &= ~fragID;
		    /*
		     * Mark the last fragment of the batch so the receiver
		     * can check for a complete message.
		     */
		    if (sendFragMask == 0) {
			fragRpcHdrPtr->flags |= RPC_LASTFRAG;
		    }
		    fragRpcHdrPtr->version =	RPC_NATIVE_VERSION;
		    fragRpcHdrPtr->flags =	rpcHdrPtr->flags;
		    fragRpcHdrPtr->clientID =	rpcHdrPtr->clientID;
		    fragRpcHdrPtr->serverID =	rpcHdrPtr->serverID;
		    fragRpcHdrPtr->channel =	rpcHdrPtr->channel;
		    fragRpcHdrPtr->serverHint =	rpcHdrPtr->serverHint;
		    fragRpcHdrPtr->bootID =	rpcHdrPtr->bootID;
		    fragRpcHdrPtr->ID =		rpcHdrPtr->ID;
		    fragRpcHdrPtr->delay =	rpcMyDelay;
		    fragRpcHdrPtr->numFrags =	nfrags;
		    fragRpcHdrPtr->fragMask =	fragID;
		    fragRpcHdrPtr->command =	rpcHdrPtr->command;

		    /*
		     * The network routines expect an array of scatter/gather
		     * elements.  The RpcBufferSet is a struct of 3 of
		     * these so we cast its address into a (Net_ScatterGather *)
		     */
		    Net_Output(spriteID, 
				(Net_ScatterGather *)&fragment[frag], 3, 
				mutexPtr);

		    /*
		     * Insert a delay after all but the last fragment.
		     * The master lock is released here to enable
		     * interrupts.  The ethernet driver needs an interrupt
		     * serviced in between the output of each packet.
		     */
		    RPC_TRACE(fragRpcHdrPtr, RPC_OUTPUT, "Fragout");
		    if (mutexPtr != (int *)NIL) {
			if (mach_AtInterruptLevel) {
			    Sys_Panic(SYS_FATAL,
			      "RpcOutput, unlocking mutex at interrupt level");
			} else {
			    MASTER_UNLOCK((*mutexPtr));
			}
		    }
		    rpcLastDelay = delay;
		    if (delay &&
			((fragRpcHdrPtr->flags & RPC_LASTFRAG) == 0)) {
			MACH_DELAY(delay);
		    }
		    if (mutexPtr != (int *)NIL) {
			MASTER_LOCK((*mutexPtr));
		    }
		}
	    }
	}
    } else {
	/*
	 * No fragmenting.
	 */
	rpcHdrPtr->numFrags = 0;
	rpcHdrPtr->paramOffset = 0;
	rpcHdrPtr->dataOffset = 0;

	if ((rpcHdrPtr->flags & RPC_TYPE) == RPC_ACK) {
	    /*
	     * Don't disturb the fragment mask.
	     */
	} else {
	    rpcHdrPtr->fragMask = 0;
	}
#ifdef TIMESTAMP
	RPC_NIL_TRACE(RPC_ETHER_OUT, "Ether output");
#endif TIMESTAMP
	Net_Output(spriteID, (Net_ScatterGather *)message, 3, mutexPtr);

	RPC_TRACE(rpcHdrPtr, RPC_OUTPUT, "Output");
    }
    return(SUCCESS);
}
