/*
 * rpcCall.c --
 *
 *      These are the top-level routines for the client side of Remote
 *      Procedure Call - the routines do overhead tasks like setting up a
 *      message, and managing client channels.  The network protocol for
 *      the client side is in rpcClient.c.
 *
 * Copyright 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */


#include "sprite.h"

#include "rpc.h"
#include "rpcClient.h"
#include "rpcTrace.h"
#include "rpcHistogram.h"

/*
 * So we can print out rpc names we include the rpcServer definitions.
 */
#include "rpcServer.h"

/*
 * The client channel table is kept as an array of pointers to channels.
 * rpcClient.h describes the contents of a ClientChannel.  The number
 * of channels limits the parallelism available on the client.  During
 * system shutdown, for example, there may be many processes doing
 * remote operations (removing swap files) at the same time.
 */

RpcClientChannel **rpcChannelPtrPtr = (RpcClientChannel **)NIL;
int		   rpcNumChannels = 8;
int		   numFreeChannels = 8;

/*
 * The allocation and freeing of channels is monitored.
 * A process might have to wait for a free RPC channel.
 */
Sync_Condition freeChannels;
Sync_Lock rpcLock;
#define LOCKPTR (&rpcLock)

/*
 * There is sequence of rpc transaction ids that increases over time.
 */

unsigned int rpcID = 1;

/*
 * A boottime Id is used to help servers realize that a client has
 * rebooted since it talked to it last.  It is zero for the first
 * RPC, one that gets the time.  Then it is set to that time and does
 * not change until we reboot.
 */

unsigned int rpcBootID = 0;
/*
 * A count is kept of the number of RPCs made.
 * The zero'th element is used to count attempts at
 * RPCs with unknown RPC numbers - RPC number 0 is unused.
 */
int rpcClientCalls[RPC_LAST_COMMAND+1];


/*
 *----------------------------------------------------------------------
 *
 * Rpc_Call --
 *
 *      Top-level interface for a client that makes a remote procedure
 *      call.  Our caller has to pre-allocate all storage for the data
 *      going to the server in the request message, and for the data
 *      returning in the reply message.  The storage argument contains
 *      pointers and sizes for this preallocated space.  Upon return
 *	the data from the reply message will be in the specified storage
 *	areas.
 *
 * Results:
 *      An error code that either reflects an error in delivery/transport
 *      or is an error code from the remote procedure.  Also, the storage
 *      input specification is modified - the return parameter and data
 *      size fields are updated to reflect the true size of the return
 *      parameter and data blocks.  Finally, the return parameter and data
 *      areas contain the results of the remote procedure call.
 *
 * Side effects:
 *	There are no side effects on this machine except that some addressing
 *	information is kept as a hint for future RPCs.  The semantics of
 *	the remote procedure are unlimited on the server machine.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Rpc_Call(serverID, command, storagePtr)
    int serverID;		/* Indicates the server for the RPC.  The
				 * special value SERVER_BROADCAST is used
				 * to specify a broadcast RPC.  The first
				 * reply received is returned and subsequent
				 * replies get discarded.  Broadcast RPCs
				 * are NOT retried if there is no response. */
    int command;		/* Rpc command.  Values defined in rpcCall.h */
    Rpc_Storage *storagePtr;	/* Specifies buffer areas for request and
				 * reply messages. */
{
    register RpcClientChannel *chanPtr;	/* Handle for communication channel */
    register ReturnStatus error;	/* General error return status */
    Time histTime;			/* Time for histogram taking */
    unsigned	int srvBootID;		/* Boot time stamp from server, used to
					 * track server reboots */
    Boolean notActive = 0;		/* Not active flag from server */

    if (serverID < 0) {
	panic("Rpc_Call, bad serverID");
	return(GEN_INVALID_ARG);
    } else if (serverID != RPC_BROADCAST_SERVER_ID &&
	       serverID == rpc_SpriteID) {
	if (command != RPC_ECHO) {
	    panic("Trying to RPC to myself");
	} else {
	    printf("Warning: Trying to RPC to myself");
	}
	return(GEN_INVALID_ARG);
    } else if ((serverID == RPC_BROADCAST_SERVER_ID) &&
	       ! (command == RPC_FS_PREFIX ||
		  command == RPC_GETTIME)) {
	panic("Trying to broadcast a non-prefix RPC");
	return(GEN_INVALID_ARG);
    } else if (serverID >= NET_NUM_SPRITE_HOSTS) {
	panic("Rpc_Call, server ID too large");
    }
#ifdef TIMESTAMP
    RPC_NIL_TRACE(RPC_CLIENT_A, "Rpc_Call");
#endif /* TIMESTAMP */

    RPC_CALL_TIMING_START(command, &histTime);
    chanPtr = RpcChanAlloc(serverID);

#ifdef TIMESTAMP
    RPC_NIL_TRACE(RPC_CLIENT_B, "alloc");
#endif /* TIMESTAMP */
    /*
     * Initialize the RPC request message header and put buffer
     * specifications of our caller into the state of the channel.  This
     * is once-per-rpc initialization that does not need to be repeated if
     * we have to re-send.
     */
    RpcSetup(serverID, command, storagePtr, chanPtr);

    /*
     * Update a histogram of RPCs made.  The zeroth element is used to
     * count unknown rpcs
     */
    if (command > 0 &&
	command <= RPC_LAST_COMMAND) {
	rpcClientCalls[command]++;
    } else {
	/*
	 */
	printf("Rpc_Call: unknown rpc command (%d)\n", command);
	rpcClientCalls[0]++;	/* 0 == RPC_BAD_COMMAND */
    }
#ifdef TIMESTAMP
    RPC_NIL_TRACE(RPC_CLIENT_C, "setup");
#endif /* TIMESTAMP */

    /*
     * Call RpcDoCall, which synchronizes with RpcClientDispatch,
     * to do the send-receive-timeout loop for the RPC.
     */
    error = RpcDoCall(serverID, chanPtr, storagePtr, command,
		      &srvBootID, &notActive);
    RpcChanFree(chanPtr);
    
#ifdef TIMESTAMP
    RPC_NIL_TRACE(RPC_CLIENT_OUT, "return");
#endif /* TIMESTAMP */

    RPC_CALL_TIMING_END(command, &histTime);
#ifndef NO_RECOVERY
    if (error == RPC_TIMEOUT || error == NET_UNREACHABLE_NET) {
	if (command != RPC_ECHO_2) {
	    printf("<%s> ", rpcService[command].name);
	    Sys_HostPrint(serverID, "RPC timed-out\n");
	}
	Recov_HostDead(serverID);
    } else {
	Recov_HostAlive(serverID, srvBootID, TRUE, notActive);
    }
#endif /* NO_RECOVERY */
    return(error);
}

/*
 *----------------------------------------------------------------------
 *
 * RpcSetup --
 *
 *      Initialize the RPC header for the request message and set up the
 *      buffer specifications for the request and reply messages.  The
 *      operations done here are done once before the request message is
 *      sent out the first time, and they do not have to be re-done if the
 *      request message needs to be re-sent.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *      The Rpc header is initialized.  The buffer specifications for the
 *      request and reply messages are set up.
 *
 *----------------------------------------------------------------------
 */
void
RpcSetup(serverID, command, storagePtr, chanPtr)
    int serverID;			/* The server for the RPC */
    int command;			/* The RPC to perform */
    register Rpc_Storage *storagePtr;	/* Specifies storage for the RPC
					 * parameters */
    register RpcClientChannel *chanPtr;	/* The channel for the RPC */
{
    register Net_ScatterGather *bufferPtr;	/* This specifies a part of the
						 * packet to the network driver
						 */
    register RpcHdr *rpcHdrPtr;			/* The RPC header */

    /*
     * Initialize the RPC header for the request message.  A couple fields
     * are set up elsewhere.  The server hint is left over from previous
     * RPCs.  The channel ID and the version number are set up at
     * boot time by Rpc_Init.
     */
    rpcHdrPtr = &chanPtr->requestRpcHdr;
    if(command == RPC_ECHO_1) {
	rpcHdrPtr->flags = RPC_ECHO;
    } else {
	rpcHdrPtr->flags = RPC_REQUEST;
    }
    rpcHdrPtr->flags |= RPC_SERVER;

    rpcHdrPtr->clientID = rpc_SpriteID;
    rpcHdrPtr->serverID = serverID;
    rpcHdrPtr->bootID = rpcBootID;
    rpcHdrPtr->ID = rpcID++;
    rpcHdrPtr->delay = rpcMyDelay;
    rpcHdrPtr->command = command;

    /*
     * Copy buffer pointers into the state of the channel.
     */
    bufferPtr		= &chanPtr->request.paramBuffer;
    bufferPtr->bufAddr	= storagePtr->requestParamPtr;
    bufferPtr->length	= storagePtr->requestParamSize;

    rpcHdrPtr->paramSize = storagePtr->requestParamSize;

    bufferPtr		= &chanPtr->request.dataBuffer;
    bufferPtr->bufAddr	= storagePtr->requestDataPtr;
    bufferPtr->length	= storagePtr->requestDataSize;

    rpcHdrPtr->dataSize = storagePtr->requestDataSize;

    bufferPtr		= &chanPtr->reply.paramBuffer;
    bufferPtr->bufAddr	= storagePtr->replyParamPtr;
    bufferPtr->length	= storagePtr->replyParamSize;

    bufferPtr		= &chanPtr->reply.dataBuffer;
    bufferPtr->bufAddr	= storagePtr->replyDataPtr;
    bufferPtr->length	= storagePtr->replyDataSize;

    /*
     * Reset state about the reception of replies.
     */
    chanPtr->actualDataSize = 0;
    chanPtr->actualParamSize = 0;
    chanPtr->fragsReceived = 0;
    chanPtr->fragsDelivered = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * RpcChanAlloc --
 *
 *      Allocate a channel for an RPC.  A pointer to the channel is
 *      returned.  The allocation is done on the basis of the server
 *      machine involved.  The goal is to send a long series of RPC
 *      requests to the same server over the same channel.  To that end a
 *      channel is chosen first if its cached address matches the input
 *      server address.  Second choice is a previously unused channel,
 *      lastly we re-use a channel that has been used with a different server.
 *
 * Results:
 *	A pointer to the channel.  This returns a good pointer, or it panics.
 *
 * Side effects:
 *	The channel is dedicated to the RPC until the caller frees
 *	the channel with RpcChanFree.
 *
 *----------------------------------------------------------------------
 */
ENTRY RpcClientChannel *
RpcChanAlloc(serverID)
    int serverID;	/* Server ID to base our allocation on. */
{
    register RpcClientChannel *chanPtr;	/* The channel we allocate */
    register int i;			/* Index into channel table */
    register int firstUnused = -1;	/* The first unused channel */
    register int firstFree = -1;	/* The first chan used but now free */

    LOCK_MONITOR;

    while (numFreeChannels < 1) {
	rpcCltStat.chanWaits++;
	(void) Sync_Wait(&freeChannels, FALSE);
    }
    
    for (i=0 ; i<rpcNumChannels ; i++) {
	chanPtr = rpcChannelPtrPtr[i];
	if (chanPtr->state == CHAN_FREE) {
	    if (chanPtr->serverID == -1) {
		/*
		 * Remember the first unused channel.
		 */
		if (firstUnused < 0) {
		    firstUnused = i;
		}
	    } else if (serverID == chanPtr->serverID) {
		/*
		 * Agreement between the channels old server and the
		 * server ID.  By reusing this channel we hope to give the
		 * server an implicit acknowledgment for the previous
		 * transaction.
		 */
		rpcCltStat.chanHits++;
		goto found;
	    } else if (firstFree < 0) {
		/*
		 * The first free channel (with some server) is less of
		 * a good candidate for allocation.
		 */
		firstFree = i;
	    }
	}
    }
    /*
     * We didn't find an address match on a free channel, so we use
     * the first previously unused channel or the first used but free channel.
     */
    if (firstUnused >= 0) {
	rpcCltStat.chanNew++;
	chanPtr = rpcChannelPtrPtr[firstUnused];
    } else if (firstFree >= 0) {
	rpcCltStat.chanReuse++;
	chanPtr = rpcChannelPtrPtr[firstFree];
    } else {
	panic("Rpc_ChanAlloc can't find the free channel.\n");
    }
    chanPtr->serverID = serverID;
found:
    chanPtr->state = CHAN_BUSY;
    numFreeChannels--;

    UNLOCK_MONITOR;
    return(chanPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * RpcChanFree --
 *
 *	Free an RPC channel.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The channel is available for other RPC calls.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
RpcChanFree(chanPtr)
    RpcClientChannel *chanPtr;		/* The channel to free */
{

    LOCK_MONITOR;

    if (chanPtr->state == CHAN_FREE) {
	panic("Rpc_ChanFree: freeing free channel\n");
    }
    chanPtr->state = CHAN_FREE;

    numFreeChannels++;
    if (numFreeChannels == 1) {
	rpcCltStat.chanBroads++;
	Sync_Broadcast(&freeChannels);
    }
    
    UNLOCK_MONITOR;
}
