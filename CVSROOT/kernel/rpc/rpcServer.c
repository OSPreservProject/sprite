/*
 * rpcServer.c --
 *
 *      This is the top level code for an RPC server process, plus the
 *      server-side dispatch routine with which the server process must
 *      synchronize.  sA server process does some initialization and then
 *      goes into a service loop receiving request messages and invoking
 *      service stubs.  This file also has utilities for sending replies
 *      and acks.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */


#include <sprite.h>
#include <stdio.h>
#include <bstring.h>
#include <rpc.h>
#include <rpcInt.h>
#include <rpcServer.h>
#include <rpcTrace.h>
#include <rpcHistogram.h>
#include <net.h>
#include <proc.h>
#include <dbg.h>
#include <stdlib.h>
#include <recov.h>

/*
 * An on/off switch for the service side of the RPC system.  Hosts do not
 * initially respond to RPC requests so they can configure themselves
 * as needed at boot time.  This means we are dependent on a user program
 * to execute and turn on this flag.
 */
Boolean rpcServiceEnabled = FALSE;

/*
 * The server state table.  It has a maximum size which constrains the
 * number of server processes that can be created.  The number of free
 * servers is maintained, as well as the total number of created servers.
 */
RpcServerState **rpcServerPtrPtr = (RpcServerState **)NIL;
int		rpcAbsoluteMaxServers = 70;
int		rpcMaxServers = 50;
int		rpcNumServers = 0;

/*
 * rpcMaxServerAge is the number of times the Rpc_Daemon will send
 * a probe message to a client (without response) before forcibly
 * reclaiming the server process for use by other clients.  A probe
 * message gets sent each time the daemon wakes up and finds the
 * server process still idle awaiting a new request from the client.
 */
int rpcMaxServerAge = 10;	/* For testing set to 1. Was 10. */

/*
 * A histogram is kept of service time.  This is available to user
 * programs via the Sys_Stats SYS_RPC_SERVER_HIST command.  The on/off
 * flags is settable via Fs_Command FS_SET_RPC_SERVER_HIST
 */
Rpc_Histogram *rpcServiceTime[RPC_LAST_COMMAND+1];
Boolean rpcServiceTiming = FALSE;

/*
 * A raw count of the number of service calls.
 */
int rpcServiceCount[RPC_LAST_COMMAND+1];

/*
 * Buffers for sending negative acknowledgements.
 */
NackData	rpcNack;
/*
 * Whether or not to send negative acknowledgements.
 */
Boolean		rpcSendNegAcks = FALSE;
/*
 * Number of nack buffers in system.
 */
int	rpc_NumNackBuffers = 4;
/*
 * To tell if we've already initialized the correct number of nack buffers
 * when the rpc system is turned on.  This variable equals the number of
 * nack buffers last initialized.
 */
int	oldNumNackBuffers = 0;


/*
 * For tracing the behavior of the rpc servers.
 */
typedef struct	RpcServerStateInfo {
    int		index;
    int		clientID;
    int		channel;
    int		state;
    int		num;
    Timer_Ticks	time;
} RpcServerStateInfo;

typedef	struct	RpcServerTraces {
    RpcServerStateInfo	*traces;
    int			traceIndex;
    Boolean		okay;
    Boolean		error;
    Sync_Semaphore	mutex;
} RpcServerTraces;

RpcServerTraces	rpcServerTraces = {(RpcServerStateInfo *) NIL, 0,  FALSE,
				FALSE, Sync_SemInitStatic("rpcServerTraces")};

/*
 * Try 1 Meg of traces.
 */
#define RPC_NUM_TRACES (0x100000 / sizeof (RpcServerStateInfo))

static void NegAckFunc _ARGS_((ClientData clientData, Proc_CallInfo *callInfoPtr));




/*
 *----------------------------------------------------------------------
 *
 * Rpc_Server --
 *
 *      The top level procedure of an RPC server process.  This
 *      synchronizes with RpcServerDispatch while it waits for new
 *      requests to service.  It assumes that its buffers contain a new
 *      request when its SRV_BUSY state bit is set.  It clears this bit
 *      and sets the SRV_WAITING bit after it completes the service
 *      procedure and returns a reply to the client.
 *
 * Results:
 *	This procedure never returns.
 *
 * Side effects:
 *	The server process attaches itself to a slot in the
 *	table of server states.  Some statistics are taken at this level.
 *
 *----------------------------------------------------------------------
 */
void
Rpc_Server()
{
    register RpcServerState *srvPtr;	/* This server's state */
    register RpcHdr *rpcHdrPtr;		/* Its request message header */
    register int command;		/* Identifies the service procedure */
    register ReturnStatus error;	/* Return error code */
    Rpc_Storage storage;		/* Specifies storage of request and
					 * reply buffers passed into the stubs*/
    Proc_ControlBlock *procPtr;		/* our process information */

    procPtr = Proc_GetCurrentProc();

    srvPtr = RpcServerInstall();
    if (srvPtr == (RpcServerState *)NIL) {
	printf("RPC server can't install itself.\n");
	Proc_Exit((int) RPC_INTERNAL_ERROR);
    }
    error = SUCCESS;
    for ( ; ; ) {
	/*
	 * Synchronize with RpcServerDispatch and await a request message.
	 * Change our state to indicate that we are ready for input.
	 */
	MASTER_LOCK(&srvPtr->mutex);
	srvPtr->state &= ~(SRV_BUSY|SRV_STUCK);
	srvPtr->state |= SRV_WAITING;
	if (error == RPC_NO_REPLY) {
	    srvPtr->state |= SRV_NO_REPLY;
	}
	while ((srvPtr->state & SRV_BUSY) == 0) {
	    Sync_MasterWait(&srvPtr->waitCondition,
				&srvPtr->mutex, TRUE);
	    if (sys_ShuttingDown) {
		srvPtr->state = SRV_NOTREADY;
		MASTER_UNLOCK(&srvPtr->mutex);
		Proc_Exit(0);
	    }
	}
	srvPtr->state &= ~SRV_NO_REPLY;
	MASTER_UNLOCK(&srvPtr->mutex);

	/*
	 * At this point there is unsynchronized access to
	 * the server state.  We are marked BUSY, however, and
	 * RpcServerDispatch knows this and doesn't muck accordingly.
	 */

	/*
	 * Free up our previous reply.  The freeReplyProc is set by the
	 * call to Rpc_Reply.
	 */
#ifndef lint
	/* Won't lint due to cast of function ptr to address. */
	if ((Address)srvPtr->freeReplyProc != (Address)NIL) {
	    (void)(*srvPtr->freeReplyProc)(srvPtr->freeReplyData);
	    srvPtr->freeReplyProc = (int (*)())NIL;
	}
#endif /* lint */

	rpcHdrPtr = &srvPtr->requestRpcHdr;
#ifdef TIMESTAMP
	RPC_TRACE(rpcHdrPtr, RPC_SERVER_A, " input");
#endif /* TIMESTAMP */
	/*
	 * Monitor message traffic to keep track of other hosts.  This call
	 * has a side effect of blocking the server process while any
	 * crash recovery call-backs are in progress.
	 */
#ifndef NO_RECOVERY
	Recov_HostAlive(srvPtr->clientID, rpcHdrPtr->bootID,
			FALSE, (Boolean) (rpcHdrPtr->flags & RPC_NOT_ACTIVE));
#endif
	/*
	 * Before branching to the service procedure we check that the
	 * server side of RPC is on, and that the RPC number is good.
	 * The "disabled service" return code is understood by other
	 * hosts to mean that we are still alive, but are not yet
	 * ready to be a server - ie. we are still checking disks etc.
	 */
	command = rpcHdrPtr->command;
	if (!rpcServiceEnabled) {
	    /*
	     * Silently ignore broadcast requests.  If we return an error
	     * we'll cause the sender to stop pre-maturely.  Otherwise
	     * return an indication that our service is not yet ready.
	     */
	    if (rpcHdrPtr->serverID == RPC_BROADCAST_SERVER_ID) {
		error = RPC_NO_REPLY;
	    } else {
		error = RPC_SERVICE_DISABLED;
	    }
	} else if (command <= 0 || command > RPC_LAST_COMMAND) {
	    error = RPC_INVALID_RPC;
	} else {
	    Time histTime;

	    rpcServiceCount[command]++;

	    if (procPtr->locksHeld != 0) {
		panic("Starting RPC with locks held.\n");
	    }

	    RPC_SERVICE_TIMING_START(command, &histTime);

	    storage.requestParamPtr	= srvPtr->request.paramBuffer.bufAddr;
	    storage.requestParamSize	= srvPtr->actualParamSize;
	    storage.requestDataPtr	= srvPtr->request.dataBuffer.bufAddr;
	    storage.requestDataSize	= srvPtr->actualDataSize;
	    storage.replyParamPtr	= (Address)NIL;
	    storage.replyParamSize	= 0;
	    storage.replyDataPtr	= (Address)NIL;
	    storage.replyDataSize	= 0;
	    error = (rpcService[command].serviceProc)((ClientData)srvPtr,
				  srvPtr->clientID, command, &storage);
	    RPC_SERVICE_TIMING_END(command, &histTime);

	    if (procPtr->locksHeld != 0) {
		panic("Finished RPC with locks held.\n");
	    }
	}
	/*
	 * Return an error reply for the stubs.  Note: We could send all
	 * replies if the stubs were all changed...
	 */
	if (error != SUCCESS && error != RPC_NO_REPLY) {
	    Rpc_ErrorReply((ClientData)srvPtr, error);
	}
#ifdef TIMESTAMP
	RPC_TRACE(rpcHdrPtr, RPC_SERVER_OUT, " done");
#endif /* TIMESTAMP */
    }
}

/*
 *----------------------------------------------------------------------
 *
 * RpcReclaimServers() --
 *
 *	Spin through the pool of server processes looking for ones to
 *	reclaimed.  A server is eligible for reclaimation if it has been
 *	idle (no new requests from its client) for a given number of
 *	passes over the pool of servers.  It is reclaimed by getting
 *	an explicit acknowledgment from the client and then marking
 *	the server as SRV_FREE.
 *
 *	WARNING: it is possible for this routine to be called very often,
 *	with the daemon being woken up by DAEMON_POKED rather than the
 *	DAEMON_TIMEOUT.  Potentially, this might not give clients enough
 *	time to do an RPC.  This doesn't seem to happen, but we should
 *	change this routine at some point to make sure it can't happen.
 *
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Age servers, send probes to idle clients, recycle reclaimied servers.
 *	If a client crashes, this routine detects it and reclaims the
 *	server process associated with it and marks the client dead.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
RpcReclaimServers(serversMaxed)
    Boolean serversMaxed;	/* TRUE if the maximum number of servers
				 * have been created.  We reclaim more
				 * quickly if this is set. */
{
    int srvIndex;
    register RpcServerState *srvPtr;
    int (*procPtr)();
    ClientData data = (ClientData) NULL;

    for (srvIndex=0 ; srvIndex < rpcNumServers ; srvIndex++) {
	srvPtr = rpcServerPtrPtr[srvIndex];

	MASTER_LOCK(&srvPtr->mutex);


	procPtr = (int (*)())NIL;
	if ((srvPtr->clientID >= 0) &&
	    (srvPtr->state & SRV_WAITING)) {
	     if (srvPtr->state & SRV_NO_REPLY) {
		/*
		 * Reclaim right away if the server process is tied
		 * up not replying to a broadcast request.
		 */
		procPtr = srvPtr->freeReplyProc;
		data = srvPtr->freeReplyData;
		srvPtr->freeReplyProc = (int (*)())NIL;
		srvPtr->freeReplyData = (ClientData)NIL;
		srvPtr->state = SRV_FREE|SRV_NO_REPLY;
		RpcAddServerTrace(srvPtr, (RpcHdr *) NIL, FALSE, 5);
	    } else if ((srvPtr->state & SRV_AGING) == 0) {
		/*
		 * This is the first pass over the server process that
		 * has found it idle.  Start it aging.
		 */
		srvPtr->state |= SRV_AGING;
		RpcAddServerTrace(srvPtr, (RpcHdr *) NIL, FALSE, 6);
		srvPtr->age = 1;
		if (serversMaxed) {
		    RpcProbe(srvPtr);
		}
	    } else {
		/*
		 * This process has aged since the last time we looked, maybe
		 * sleepTime ago.  We resend to the client with the
		 * close flag and continue on.  If RpcServerDispatch gets
		 * a reply from the client closing its connection it will
		 * mark the server process SRV_FREE.  If we continue to
		 * send probes with no reply, we give up after N tries
		 * and free up the server.  It is possible that the client
		 * has re-allocated its channel, in which case it drops
		 * our probes on the floor, or that it has crashed.
		 */
		srvPtr->age++;
		if (srvPtr->age >= rpcMaxServerAge) {
		    procPtr = srvPtr->freeReplyProc;
		    data = srvPtr->freeReplyData;
		    srvPtr->freeReplyProc = (int (*)())NIL;
		    srvPtr->freeReplyData = (ClientData)NIL;
		    rpcSrvStat.reclaims++;
		    srvPtr->state = SRV_FREE;
		    RpcAddServerTrace(srvPtr, (RpcHdr *) NIL, FALSE, 7);
#ifdef notdef
		} else if (srvPtr->clientID == rpc_SpriteID) {
		    printf("Warning: Reclaiming from myself.\n");
		    srvPtr->state = SRV_FREE;
		    RpcAddServerTrace(srvPtr, (RpcHdr *) NIL, FALSE, 8);
#endif
		} else {
		    /*
		     * Poke at the client to get a response.
		     */
		    RpcProbe(srvPtr);
		}
	    }
	} else if ((srvPtr->state & SRV_FREE) &&
		   (srvPtr->freeReplyProc != (int (*)())NIL)) {
	    /*
	     * Tidy up after an explicit acknowledgment from a client.
	     * This can't be done at interrupt time by ServerDispatch.
	     */
	    procPtr = srvPtr->freeReplyProc;
	    data = srvPtr->freeReplyData;
	    srvPtr->freeReplyProc = (int (*)())NIL;
	    srvPtr->freeReplyData = (ClientData)NIL;
	}
	MASTER_UNLOCK(&srvPtr->mutex);
	/*
	 * Do the call-back to free up resources associated with the last RPC.
	 */
	if (procPtr != (int (*)())NIL) {
	    (void)(*procPtr)(data);
	}
    }
}



/*
 *----------------------------------------------------------------------
 *
 * NegAckFunc --
 *
 *	Call-back to send a negative acknowledgement so that we won't be at
 *	interrupt level while doing this.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A negative ack is output.
 *
 *----------------------------------------------------------------------
 */
static void
NegAckFunc(clientData, callInfoPtr)
    ClientData		clientData;
    Proc_CallInfo	*callInfoPtr;
{
    int			i;

    MASTER_LOCK(&(rpcNack.mutex));
    /*
     * We may handle more than one request here.
     */
    for (i = 0; i < rpc_NumNackBuffers; i++) {
	if (rpcNack.hdrState[i] == RPC_NACK_WAITING) {
	    rpcNack.hdrState[i] = RPC_NACK_XMITTING;
	    rpcNack.rpcHdrArray[i].flags = RPC_NACK | RPC_ACK;
	    /*
	     * Already did an RpcSrvInitHdr from incoming rpcHdrPtr to our
	     * outgoing * buffer in RpcServerDispatch.
	     */
	    /*
	     * This should be okay to do under a masterlock since RpcAck
	     * also calls it and it's under a masterlock.
	     */
	    RpcAddServerTrace((RpcServerState *) NIL, &(rpcNack.rpcHdrArray[i]),
		    TRUE, 19);
	    rpcSrvStat.nacks++;
	    /*
	     * Because we pass it the mutex, it will return only when the xmit
	     * is done.
	     */
	    (void) RpcOutput(rpcNack.rpcHdrArray[i].clientID,
		    &(rpcNack.rpcHdrArray[i]), &(rpcNack.bufferSet[i]),
		    (RpcBufferSet *) NIL, 0,
		    (Sync_Semaphore *) &(rpcNack.mutex));
	    rpcNack.hdrState[i] = RPC_NACK_FREE;
	    rpcNack.numFree++;
	}
    }
    MASTER_UNLOCK(&(rpcNack.mutex));
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * RpcServerDispatch --
 *
 *      Handle a message sent to a server process.  The client sends (and
 *      re-sends) request messages to the server host.  This routine
 *      handles incoming message accoring to the RPC protocol.  Only new
 *      request messages are passed to the server process.  All other
 *      messages are handled in this routine.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *      The side effects depend on the type of message received.  New
 *      requests are passed off to server processes, client acknowledgments
 *      and retries are processed and discarded.  Unneeded messages are
 *	discarded by returning without copying the message out of the
 *	network buffers.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
RpcServerDispatch(srvPtr, rpcHdrPtr)
    register RpcServerState *srvPtr;	/* The state of the server process */
    register RpcHdr *rpcHdrPtr;		/* The header of the packet as it sits
					 * in the hardware buffers */
{
    register int size;		/* The amount of the data in the message */
    int		i;
    int		foundSpot;	/* Neg ack buf to use. */
    int		alreadyFunc;	/* What buf neg ack function is dealing with. */


    /*
     * If the server pointer is NIL, this means no server could be allocated
     * and a negative acknowledgement must be issued.
     */
    if (srvPtr == (RpcServerState *) NIL) {
	if (rpcHdrPtr->clientID == rpc_SpriteID) {
	    /*
	     * Can't nack myself since the network module turns around and
	     * reroutes the message and we get deadlock.  Drop the neg ack.
	     */
	    printf("Can't nack to myself!\n");
	    rpcSrvStat.selfNacks++;
	    return;
	}
	MASTER_LOCK(&(rpcNack.mutex));
	if (rpc_NumNackBuffers - rpcNack.numFree >
	    rpcSrvStat.mostNackBuffers) {
	    rpcSrvStat.mostNackBuffers = rpc_NumNackBuffers - rpcNack.numFree;
	}
	if (rpcNack.numFree <= 0) {
	    /* Drop the negative ack. */
	    MASTER_UNLOCK(&(rpcNack.mutex));
	    return;
	}
	/*
	 * Copy rpc header info to safe place that won't be freed when
	 * we return.
	 */
	alreadyFunc = -1;
	foundSpot = -1;
	for (i = 0; i < rpc_NumNackBuffers; i++) {
	    /*
	     * If we haven't already found a buffer to use and this one is
	     * free, use it.
	     */
	    if (foundSpot == -1 && rpcNack.hdrState[i] == RPC_NACK_FREE) {
		rpcNack.numFree--;
		rpcNack.hdrState[i] = RPC_NACK_WAITING;
		foundSpot = i;
		RpcSrvInitHdr((RpcServerState *) NIL, 
				&(rpcNack.rpcHdrArray[i]), rpcHdrPtr);
	    /*
	     * If we've found evidence that there's already a CallFunc for
	     * the NegAckFunc, then record the first buffer it will be dealing
	     * with next.
	     */
	    } else if (alreadyFunc == -1 &&
		    rpcNack.hdrState[i] == RPC_NACK_WAITING) {
		alreadyFunc = i;
	    }
	}
	/*
	 * If we've found a buffer and either there's no CallFunc or else it
	 * is already past the buffer we've grabbed, then start another
	 * CallFunc.
	 */
	if (foundSpot != -1 &&
		(alreadyFunc == -1 || alreadyFunc >= foundSpot)) {
	    MASTER_UNLOCK(&(rpcNack.mutex));
	    Proc_CallFunc(NegAckFunc, (ClientData) NIL, 0);
	    return;
	/*
	 * Otherwise if we've found a buffer, there's already a CallFunc.
	 */
	} else if (foundSpot != -1) {
	    MASTER_UNLOCK(&(rpcNack.mutex));
	    return;
	}
	MASTER_UNLOCK(&(rpcNack.mutex));
	/*
	 * If we haven't found a free buffer, something is wrong.
	 */
	panic("RpcServerDispatch: couldn't find free rpcHdr.\n");
    }

    /*
     * Acquire the server's mutex for multiprocessor synchronization.  We
     * synchronize with each server process with a mutex that is part of
     * the server's state.
     */
    MASTER_LOCK(&srvPtr->mutex);
    
#ifdef TIMESTAMP
    RPC_TRACE(rpcHdrPtr, RPC_SERVER_a, " server");
#endif /* TIMESTAMP */
    /*
     * Reset aging servers.  This information is maintained by Rpc_Deamon.
     * The reception of a message for the server makes it no longer idle.
     */
    srvPtr->state &= ~SRV_AGING;
    srvPtr->age = 0;

    /*
     * If the RPC sequence number is different than the one saved in the
     * server's state then this request signals a new RPC.  We use this as
     * an implicit acknowledgment of the last reply the server sent.  The
     * last transaction ID is saved in the rpc header of our last reply
     * message.
     */
    if (rpcHdrPtr->ID != srvPtr->ID) {

	rpcSrvStat.requests++;
	if (srvPtr->state & SRV_WAITING) {
	    /*
	     * The server has computed a result already so we treat this
	     * new request as an implicit acknowledgment of the reply the
	     * server sent.  The server process will clean up the previous
	     * reply before it starts computing the new reply.
	     */
	    rpcSrvStat.impAcks++;
	    srvPtr->state &= ~SRV_WAITING;
	}

	if (srvPtr->state & SRV_FRAGMENT) {
	    /*
	     * The last send by the client was fragmented but was aborted
	     * before we got all the fragments.  Reset the fragment
	     * reasembly process.
	     */
	    rpcSrvStat.fragAborts++;
	    srvPtr->state &= ~SRV_FRAGMENT;
	    srvPtr->fragsReceived = 0;
	}

	if (srvPtr->state & SRV_BUSY) {
	    /*
	     * The client has abandoned the RPC and started on a new one.
	     * This server process may be stuck on some lock, or the
	     * client may just be in error.  We mark the server process
	     * as STUCK so subsequent requests will not use this server,
	     * and the server process will unmark itself when it completes
	     * what ever its working on.
	     */
	    rpcSrvStat.serverBusy++;
	    srvPtr->state |= SRV_STUCK;
	    goto unlock;
	}
	/*
	 * (There should be no bits set in the server's state word.)
	 *
	 * Update the server's idea of the current transaction.
	 */
	srvPtr->ID = rpcHdrPtr->ID;
	/*
	 * Reset the true sizes of the two data areas in the arriving message.
	 */
	srvPtr->actualDataSize = 0;
	srvPtr->actualParamSize = 0;
	/*
	 * Reset our knowledge of what fragments our client has.
	 */
	srvPtr->fragsDelivered = 0;
	/*
	 * Copy the message from the network's buffers into those of
	 * the server process.
	 */
	RpcScatter(rpcHdrPtr, &srvPtr->request);

	/*
	 * Note the actual size of the input information.
	 */
	size = rpcHdrPtr->paramSize + rpcHdrPtr->paramOffset;
	if (srvPtr->actualParamSize < size) {
	    srvPtr->actualParamSize = size;
	}
	size = rpcHdrPtr->dataSize + rpcHdrPtr->dataOffset;
	if (srvPtr->actualDataSize < size) {
	    srvPtr->actualDataSize = size;
	}
	if (rpcHdrPtr->numFrags == 0) {
	    /*
	     * The message is complete, ie. not fragmented.
	     * Return ack if needed and notify the server process.
	     */
	    srvPtr->fragsReceived = 0;
	    if (rpcHdrPtr->flags & RPC_PLSACK) {
		rpcSrvStat.handoffAcks++;
		RpcAck(srvPtr, 0);
	    }
	    srvPtr->state = SRV_BUSY;
#ifdef WOULD_LIKE
	    RpcAddServerTrace(srvPtr, (RpcHdr *) NIL, FALSE, 2);
#endif WOULD_LIKE
	    rpcSrvStat.handoffs++;
	    Sync_MasterBroadcast(&srvPtr->waitCondition);
	} else {
	    /*
	     * The new arrival is only a fragment.
	     * Initiate fragment reassembly.  The server process is not
	     * notified until the request message is complete.
	     */
	    rpcSrvStat.fragMsgs++;
	    srvPtr->state = SRV_FRAGMENT;
	    srvPtr->fragsReceived = rpcHdrPtr->fragMask;
	    if (rpcHdrPtr->flags & RPC_PLSACK) {
		rpcSrvStat.fragAcks++;
		RpcAck(srvPtr, RPC_LASTFRAG);
	    }
	}
#ifdef TIMESTAMP
	RPC_TRACE(rpcHdrPtr, RPC_SERVER_b, "handoff");
#endif /* TIMESTAMP */
    } else {
	/*
	 * This is a message concerning a current RPC.
	 */
	if (srvPtr->state & SRV_NO_REPLY) {
	    /*
	     * The current RPC was a broadcast which we do not want
	     * to reply to.  Keep ourselves free - the allocation routine
	     * has cleared that state bit.
	     */
	    srvPtr->state |= SRV_FREE;
	    RpcAddServerTrace(srvPtr, (RpcHdr *) NIL, FALSE, 9);
	    rpcSrvStat.discards++;
	} else if (rpcHdrPtr->flags & RPC_ACK) {
	    if (rpcHdrPtr->flags & RPC_CLOSE) {
		/*
		 * This is an explicit acknowledgment to our reply.
		 * Free the server regardless of its state.  The call-back
		 * procedure to free resources will be called the next
		 * time the server process gets a request, or by Rpc_Daemon.
		 */
		rpcSrvStat.closeAcks++;
		srvPtr->state = SRV_FREE;
		RpcAddServerTrace(srvPtr, (RpcHdr *) NIL, FALSE, 10);
	    } else if (rpcHdrPtr->flags & RPC_LASTFRAG) {
		/*
		 * This is a partial acknowledgment.  The fragMask field
		 * has the summary bitmask of the client which indicates
		 * what fragments the client has received.
		 */
		rpcSrvStat.recvPartial++;
		srvPtr->fragsDelivered = rpcHdrPtr->fragMask;
		RpcResend(srvPtr);
	    } else {
		/*
		 * Do nothing. Unknown kind of ack.
		 */
		rpcSrvStat.unknownAcks++;
	    }
	} else {
	    /*
	     * Process another fragment or a re-sent request.
	     */
	    switch(srvPtr->state) {
		default:
		    /*
		     * oops.  A client is using the same RPC ID that it
		     * used with use last (as it crashed, probably).
		     * Reset srvPtr->replyRpcHdr.ID so that we can accept
		     * new requests from the client.
		     */
#ifdef notdef
	    printf("Unexpected Server state %x, idx %d, Rpc ID <%x> flags <%x> clientID %d\n",
			    srvPtr->index, srvPtr->state, rpcHdrPtr->ID,
			    rpcHdrPtr->flags, rpcHdrPtr->clientID);
#endif
		    rpcSrvStat.badState++;
		    srvPtr->replyRpcHdr.ID = 0;
		    srvPtr->state = SRV_FREE;
		    break;
		case SRV_FREE:
		    /*
		     * This is an extra packet that has arrived after the
		     * client has explicitly acknowledged its reply.
		     */
		    rpcSrvStat.extra++;
		    break;
		case SRV_FRAGMENT:
		    /*
		     * Sanity check to make sure we expect fragments
		     * and then continue fragment reasembly.
		     */
		    if (rpcHdrPtr->fragMask == 0 ||
			rpcHdrPtr->numFrags == 0) {
			rpcSrvStat.nonFrag++;
			printf("ServerDispatch - got a non-fragment\n");
			break;
		    }
		    if (srvPtr->fragsReceived & rpcHdrPtr->fragMask) {
			/*
			 * Duplicate Fragment.  This ack will return
			 * the srvPtr->fragsReceived bitmask to the client.
			 */
			rpcSrvStat.dupFrag++;
			RpcAck(srvPtr, RPC_LASTFRAG);
			break;
		    } else {
			/*
			 * Copy in new fragment and check for completion.
			 */
			rpcSrvStat.reassembly++;
			RpcScatter(rpcHdrPtr, &srvPtr->request);
			/*
			 * Update actual size information
			 */
			size = rpcHdrPtr->paramSize + rpcHdrPtr->paramOffset;
			if (srvPtr->actualParamSize < size) {
			    srvPtr->actualParamSize = size;
			}
			size = rpcHdrPtr->dataSize + rpcHdrPtr->dataOffset;
			if (srvPtr->actualDataSize < size) {
			    srvPtr->actualDataSize = size;
			}

			srvPtr->fragsReceived |= rpcHdrPtr->fragMask;
			if (srvPtr->fragsReceived ==
			    rpcCompleteMask[rpcHdrPtr->numFrags]) {
			    srvPtr->state = SRV_BUSY;
			    RpcAddServerTrace(srvPtr, (RpcHdr *) NIL, FALSE, 3);
			    rpcSrvStat.handoffs++;
			    Sync_MasterBroadcast(&srvPtr->waitCondition);
			} else if (rpcHdrPtr->flags & RPC_LASTFRAG) {
			    /*
			     * Incomplete packet after we've gotten
			     * the last fragment in the series.  Partial Ack.
			     */
			    rpcSrvStat.sentPartial++;
			    RpcAck(srvPtr, RPC_LASTFRAG);
			} else {
			    /*
			     * Ignore any "please ack" requests here.  If
			     * the client is resending it will already
			     * trigger a partial ack on every duplicate
			     * fragment we get.
			     */
			}
		    }
		    break;
		case SRV_BUSY:
		    /*
		     * We already got this request and the server is busy
		     * with it.  We send an explicit acknowledgment to the
		     * client to let it know that we successfully got the
		     * request.
		     */
		    rpcSrvStat.busyAcks++;
		    RpcAck(srvPtr, 0);
		    RpcAddServerTrace(srvPtr, (RpcHdr *) NIL, FALSE, 18);
		    break;
		case SRV_WAITING:
		    /*
		     * The client has dropped our reply, we resend it.
		     */
		    rpcSrvStat.resends++;
		    RpcResend(srvPtr);
		    break;
	    }
	}
#ifdef TIMESTAMP
	RPC_TRACE(rpcHdrPtr, RPC_SERVER_c, "return");
#endif /* TIMESTAMP */
    }
unlock:
    MASTER_UNLOCK(&srvPtr->mutex);
}

/*
 *----------------------------------------------------------------------
 *
 * Rpc_ErrorReply --
 *
 *	Return an error code and an empty reply to a client.
 *
 * Results:
 *	Transmits an error reply to the client host.
 *
 * Side effects:
 *	Send the packet.  Clears the freeReplyProc and Data because
 *	there is an empty reply.
 *
 *----------------------------------------------------------------------
 */
void
Rpc_ErrorReply(srvToken, error)
    ClientData srvToken;		/* Opaque token passed to stub */
    int error;				/* Error code to return to client */
{
    RpcServerState *srvPtr;
    register RpcHdr	*rpcHdrPtr;
    register RpcHdr	*requestHdrPtr;

    srvPtr = (RpcServerState *)srvToken;
    rpcHdrPtr = &srvPtr->replyRpcHdr;
    requestHdrPtr = &srvPtr->requestRpcHdr;

    srvPtr->freeReplyProc = (int (*)())NIL;
    srvPtr->freeReplyData = (ClientData)NIL;

    RpcSrvInitHdr(srvPtr, rpcHdrPtr, requestHdrPtr);

    /*
     * Communicate the error code back in the command feild.
     */
    rpcHdrPtr->command = error;
    rpcHdrPtr->flags = RPC_REPLY | RPC_ERROR;

    /*
     * Clear sizes in the reply buffers, but not the addresses.
     * This forces a null return and our caller can free the data.
     */
    srvPtr->reply.paramBuffer.length = 0;
    srvPtr->reply.dataBuffer.length = 0;

    (void)RpcOutput(rpcHdrPtr->clientID, rpcHdrPtr, &srvPtr->reply,
			 (RpcBufferSet *)NIL, 0, (Sync_Semaphore *)NIL);
}


/*
 *----------------------------------------------------------------------
 *
 * RpcSrvInitHdr --
 *
 *	Initialize the header of a server message from fields
 *	in the clients request message.  This relies on initialization
 *	of unchanging fields inside RpcBufferInit.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Addressing and sequencing information from the clients request
 *	message is copeid into the header of a server's outgoing message.
 *
 *----------------------------------------------------------------------
 */
void
RpcSrvInitHdr(srvPtr, rpcHdrPtr, requestHdrPtr)
    RpcServerState	*srvPtr;
    RpcHdr		*rpcHdrPtr;	/* header of outgoing message */
    RpcHdr		*requestHdrPtr;	/* header of client's request */
{
    rpcHdrPtr->serverID = rpc_SpriteID;
    rpcHdrPtr->clientID = requestHdrPtr->clientID;
    rpcHdrPtr->channel = requestHdrPtr->channel;
    rpcHdrPtr->bootID = rpcBootID;
    rpcHdrPtr->ID = requestHdrPtr->ID;
    rpcHdrPtr->numFrags = 0;
    rpcHdrPtr->fragMask = 0;
    rpcHdrPtr->command = requestHdrPtr->command;
    rpcHdrPtr->paramSize = 0;
    rpcHdrPtr->dataSize = 0;

    if (srvPtr == (RpcServerState *) NIL) {
	/*
	 * If this is a negative ack due to no server proc being available,
	 * make sure the serverHint value is reasonable so we don't mess up
	 * the client.
	 */
	rpcHdrPtr->serverHint = 0;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Rpc_Reply --
 *
 *	Return a reply to a client.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Send the reply.
 *
 *----------------------------------------------------------------------
 */
void
Rpc_Reply(srvToken, error, storagePtr, freeReplyProc, freeReplyData)
    ClientData		srvToken;		/* The token for the server
						 * passed to the stub from
						 * the server process */
    int			error;			/* Error code, or SUCCESS */
    register Rpc_Storage *storagePtr;		/* Only the reply fields are
    						 * significant. */
    int			(*freeReplyProc) _ARGS_((ClientData freeReplyData));
						/* Procedure to call to free
						 * up reply state, or NIL */
    ClientData		freeReplyData;		/* Passed to freeReplyProc */
{
    RpcServerState	*srvPtr;
    register RpcHdr	*rpcHdrPtr;
    register RpcHdr	*requestHdrPtr;


    srvPtr = (RpcServerState *)srvToken;
    rpcHdrPtr = &srvPtr->replyRpcHdr;
    requestHdrPtr = &srvPtr->requestRpcHdr;

    /*
     * Set up the call back that will free resources associated
     * with the reply.
     */
    srvPtr->freeReplyProc = freeReplyProc;
    srvPtr->freeReplyData = freeReplyData;
    
    RpcSrvInitHdr(srvPtr, rpcHdrPtr, requestHdrPtr);
    rpcHdrPtr->flags = RPC_REPLY;
    if (error) {
	/*
	 * Communicate the error code back in the command field.
	 */
	rpcHdrPtr->command = error;
	rpcHdrPtr->flags |= RPC_ERROR;
    }

    /*
     * Copy buffer pointers into the server's state.
     */
    rpcHdrPtr->paramSize = storagePtr->replyParamSize;
    srvPtr->reply.paramBuffer.length = storagePtr->replyParamSize;
    srvPtr->reply.paramBuffer.bufAddr = storagePtr->replyParamPtr;

    rpcHdrPtr->dataSize = storagePtr->replyDataSize;
    srvPtr->reply.dataBuffer.length = storagePtr->replyDataSize;
    srvPtr->reply.dataBuffer.bufAddr = storagePtr->replyDataPtr;

    (void)RpcOutput(rpcHdrPtr->clientID, rpcHdrPtr, &srvPtr->reply,
			 srvPtr->fragment, 0, (Sync_Semaphore *)NIL);
}



/*
 *----------------------------------------------------------------------
 *
 * RpcAck --
 *
 *	Return an explicit acknowledgment to a client.
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
RpcAck(srvPtr, flags)
    RpcServerState *srvPtr;
    int flags;
{
    RpcHdr	*ackHdrPtr;
    RpcHdr	*requestHdrPtr;

    ackHdrPtr = &srvPtr->ackRpcHdr;
    requestHdrPtr = &srvPtr->requestRpcHdr;

    ackHdrPtr->flags = flags | RPC_ACK;
    RpcSrvInitHdr(srvPtr, ackHdrPtr, requestHdrPtr);
    /*
     * Let the client know what fragments we have received
     * so it can optimize retransmission.
     */
    ackHdrPtr->fragMask = srvPtr->fragsReceived;
    /*
     * Note, can't try ARP here because of it's synchronization with
     * a master lock and because we are called at interrupt time.
     */
    (void)RpcOutput(ackHdrPtr->clientID, ackHdrPtr, &srvPtr->ack,
			 (RpcBufferSet *)NIL, 0, (Sync_Semaphore *)NIL);
}

/*
 *----------------------------------------------------------------------
 *
 * RpcResend --
 *
 *	Resend a reply to a client.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Send the reply.
 *
 *----------------------------------------------------------------------
 */
void
RpcResend(srvPtr)
    RpcServerState	*srvPtr;
{
    /*
     * Consistency check against a service stub that forgot to send a reply.
     * We can't check sequence numbers because RpcServerDispatch updates the
     * reply sequence number, but we can verify that the command
     * in the reply matches the command in the reply.
     */
    if ((srvPtr->replyRpcHdr.flags & RPC_ERROR) == 0 &&
	(srvPtr->replyRpcHdr.command != srvPtr->requestRpcHdr.command)) {
	printf("RpcResend: RPC %d, client %d, RPC seq # %x, forgot reply?\n",
	    srvPtr->requestRpcHdr.command, srvPtr->requestRpcHdr.clientID,
	    srvPtr->requestRpcHdr.ID);
	return;
    }
    (void)RpcOutput(srvPtr->replyRpcHdr.clientID, &srvPtr->replyRpcHdr,
		       &srvPtr->reply, srvPtr->fragment,
		       srvPtr->fragsDelivered, (Sync_Semaphore *)NIL);
}

/*
 *----------------------------------------------------------------------
 *
 * RpcProbe --
 *
 *	Send a probe message to the client to see if it is still interested
 *	in the connection.  The Ack message header/buffer is used for this.
 *	Synchronization note.  We are called with the srvPtr mutex locked,
 *	but the server process is not marked BUSY so the dispatcher might
 *	try to allocate this server.  The mutex prevents this, but it
 *	is important to not pass the mutex to RpcOutput, which would use
 *	it to wait for output of the packet.  In that case the mutex is
 *	released for a while, leaving a window of vulnerability where the
 *	server could get allocated and the mutex grabbed by the dispatcher.
 *	By not passing the mutex the packet is sent asynchronously, so there
 *	is a very remote chance we could try to issue another probe while
 *	this is still in the output queue.  However, we would be sending
 *	the same information in the packet, so there shouldn't be a problem.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Send the probe message.
 *
 *----------------------------------------------------------------------
 */
void
RpcProbe(srvPtr)
    RpcServerState	*srvPtr;
{
    RpcHdr	*ackHdrPtr;
    RpcHdr	*requestHdrPtr;

    ackHdrPtr = &srvPtr->ackRpcHdr;
    requestHdrPtr = &srvPtr->requestRpcHdr;

    ackHdrPtr->flags = RPC_ACK | RPC_CLOSE;
    RpcSrvInitHdr(srvPtr, ackHdrPtr, requestHdrPtr);

    (void)RpcOutput(ackHdrPtr->clientID, ackHdrPtr, &srvPtr->ack,
			 (RpcBufferSet *)NIL, 0, (Sync_Semaphore *)NIL);
}


/*
 *----------------------------------------------------------------------
 *
 * RpcAddServerTrace --
 *
 *	Add another trace to the list of state tracing for rpc servers.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The list is lengthened.
 *
 *----------------------------------------------------------------------
 */
void
RpcAddServerTrace(srvPtr, rpcHdrPtr, noneThere, num)
    RpcServerState	*srvPtr;	/* State to record */
    RpcHdr 		*rpcHdrPtr;	/* The request message header */
    Boolean		noneThere;	/* No server free */
    int			num;		/* Which trace */
{
    RpcServerStateInfo	*rpcTracePtr;

    if (!rpcServerTraces.okay) {
	return;
    }
    rpcTracePtr = &(rpcServerTraces.traces[rpcServerTraces.traceIndex]);
    (void) bzero((Address) rpcTracePtr, sizeof (RpcServerStateInfo));
    if (!noneThere) {
	rpcTracePtr->index = srvPtr->index;
	rpcTracePtr->clientID = srvPtr->clientID;
	rpcTracePtr->channel = srvPtr->channel;
	rpcTracePtr->state = srvPtr->state;
    } else {
	rpcTracePtr->index = -1;
	rpcTracePtr->clientID = rpcHdrPtr->clientID;
	rpcTracePtr->channel = rpcHdrPtr->channel;
	rpcTracePtr->state = rpcHdrPtr->command;
    }
    rpcTracePtr->num = num;
    Timer_GetCurrentTicks(&rpcTracePtr->time);
    rpcServerTraces.traceIndex++;
    if (rpcServerTraces.traceIndex >= RPC_NUM_TRACES) {
	rpcServerTraces.okay = FALSE;
	rpcServerTraces.error = TRUE;
    }

    return;
}


/*
 *----------------------------------------------------------------------
 *
 * Rpc_OkayToTrace --
 *
 *	Okay to turn on rpc server state tracing.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The tracing is turned on.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
Rpc_OkayToTrace(okay)
    Boolean	okay;
{
    MASTER_LOCK(&(rpcServerTraces.mutex));
    if (okay) {
	if (!rpcServerTraces.error) {
	    rpcServerTraces.okay = okay;
	}
    } else {
	rpcServerTraces.okay = okay;
    }
    MASTER_UNLOCK(&(rpcServerTraces.mutex));

    return;
}


/*
 *----------------------------------------------------------------------
 *
 * Rpc_FreeTraces --
 *
 *	If memory allocation is involved, free up the space used by the rpc
 *	server tracing.  Otherwise, just reinitialize it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Tracing is turned off and some reinitialization is done.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
Rpc_FreeTraces()
{
    MASTER_LOCK(&(rpcServerTraces.mutex));
    rpcServerTraces.okay = FALSE;

    rpcServerTraces.traceIndex = 0;
    rpcServerTraces.error = FALSE;

    MASTER_UNLOCK(&(rpcServerTraces.mutex));
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * Rpc_DumpServerTraces --
 *
 *	Dump the server traces into a buffer for the user.
 *
 * Results:
 *	Failure if something goes wrong.  Success otherwise.
 *
 * Side effects:
 *	The trace info is copied into a buffer.  Size of needed buffer is
 *	also copied out.
 *
 *----------------------------------------------------------------------
 */
ENTRY ReturnStatus
Rpc_DumpServerTraces(length, resultPtr, lengthNeededPtr)
    int                 	length;         /* size of data buffer */
    RpcServerUserStateInfo	*resultPtr;	/* Array of info structs. */
    int                 	*lengthNeededPtr;/* to return space needed */

{
    RpcServerUserStateInfo	*infoPtr;
    RpcServerStateInfo		*itemPtr;
    int				numNeeded;
    int				numAvail;
    int				i;

    MASTER_LOCK(&(rpcServerTraces.mutex));
    if (rpcServerTraces.traceIndex <= 0) {
	MASTER_UNLOCK(&(rpcServerTraces.mutex));
	return FAILURE;
    }
    if (resultPtr != (RpcServerUserStateInfo *) NIL) {
	bzero((char *) resultPtr, length);
    }
    numNeeded = 0;
    numAvail = length / sizeof (RpcServerUserStateInfo);

    infoPtr = resultPtr;
    for (i = 0; i < rpcServerTraces.traceIndex; i++) {
	itemPtr = &(rpcServerTraces.traces[i]);
	numNeeded++;
	if (numNeeded > numAvail) {
	    continue;
	}
	infoPtr->index = itemPtr->index;
	infoPtr->clientID = itemPtr->clientID;
	infoPtr->channel = itemPtr->channel;
	infoPtr->state = itemPtr->state;
	infoPtr->num = itemPtr->num;
	Timer_GetRealTimeFromTicks(itemPtr->time, &(infoPtr->time), 
				(int *) NIL, (Boolean *) NIL);
	infoPtr++;
    }
    *lengthNeededPtr = numNeeded * sizeof (RpcServerUserStateInfo);

    MASTER_UNLOCK(&(rpcServerTraces.mutex));
    return SUCCESS;
}

void
RpcInitServerTraces()
{
    rpcServerTraces.traces = (RpcServerStateInfo *) malloc(RPC_NUM_TRACES *
						sizeof (RpcServerStateInfo));
    return;
}



/*
 *----------------------------------------------------------------------
 *
 * RpcSetNackBufs --
 *
 *	Allocate and set up the correct number of nack buffers.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Some freeing and malloc'ing.
 *
 *----------------------------------------------------------------------
 */
void
RpcSetNackBufs()
{
    int	i;

    if (oldNumNackBuffers == rpc_NumNackBuffers) {
	/* Nothing to do, initialized already. */
	return;
    }
    if (rpcServiceEnabled) {
	/* Cannot change nack buffers once service is enabled, for now. */
	return;
    }
    /* Free old nack buffers if there were any. */
    if (oldNumNackBuffers > 0 && rpcNack.rpcHdrArray != (RpcHdr *) NIL) {
	free((char *) rpcNack.rpcHdrArray);
	free((char *) rpcNack.hdrState);
	free((char *) rpcNack.bufferSet);
	/*
	 * Note, this won't free up all the buffer stuff since
	 * RpcBufferInit calls Vm_RawAlloc instead of malloc.
	 */
    }
    /* Allocate new nack buffers. */
    rpcNack.rpcHdrArray = (RpcHdr *) malloc(rpc_NumNackBuffers *
	    sizeof (RpcHdr));
    rpcNack.hdrState = (int *) malloc(rpc_NumNackBuffers * sizeof (int));

    rpcNack.bufferSet = (RpcBufferSet *) malloc(rpc_NumNackBuffers *
            sizeof (RpcBufferSet));

    for (i = 0; i < rpc_NumNackBuffers; i++) {
        rpcNack.hdrState[i] = RPC_NACK_FREE;
        RpcBufferInit(&(rpcNack.rpcHdrArray[i]),
                &(rpcNack.bufferSet[i]), -1, -1);
    }
    /* set old to new */
    rpcNack.numFree = rpc_NumNackBuffers;
    oldNumNackBuffers = rpc_NumNackBuffers;

    return;
}
