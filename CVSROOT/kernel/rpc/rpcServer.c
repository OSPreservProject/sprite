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
#endif not lint


#include "sprite.h"
#include "rpc.h"
#include "rpcInt.h"
#include "rpcServer.h"
#include "rpcTrace.h"
#include "rpcHistogram.h"
#include "net.h"
#include "proc.h"
#include "dbg.h"
#include "mem.h"

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
int              rpcMaxServers = 50;
int              rpcNumServers = 0;

/*
 * The server dispatcher signals its distress at not being able to dispatch
 * a message because there are no server processes by incrementing this
 * counter.  Rpc_Deamon notices this and creates more server processes.
 */
int		 rpcNoServers = 0;

/*
 * rpcMaxServerAge is the number of times the Rpc_Daemon will send
 * a probe message to a client (without response) before forcibly
 * reclaiming the server process for use by other clients.  A probe
 * message gets sent each time the daemon wakes up and finds the
 * server process still idle awaiting a new request from the client.
 */
int rpcMaxServerAge = 10;

/*
 * A histogram is kept of service time.  This is available to user
 * programs via the Sys_Stats SYS_RPC_SERVER_HIST command.  The on/off
 * flags is settable via Fs_Command FS_SET_RPC_SERVER_HIST
 */
Rpc_Histogram *rpcServiceTime[RPC_LAST_COMMAND+1];
Boolean rpcServiceTiming = FALSE;


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

    srvPtr = RpcServerInstall();
    if (srvPtr == (RpcServerState *)NIL) {
	Sys_Printf("RPC server can't install itself.\n");
	Proc_Exit(RPC_INTERNAL_ERROR);
    }
    error = SUCCESS;
    for ( ; ; ) {
	/*
	 * Synchronize with RpcServerDispatch and await a request message.
	 * Change our state to indicate that we are ready for input.
	 */
	MASTER_LOCK(srvPtr->mutex);
	srvPtr->state &= ~SRV_BUSY;
	srvPtr->state |= SRV_WAITING;
	if (error == RPC_NO_REPLY) {
	    srvPtr->state |= SRV_NO_REPLY;
	}
	while ((srvPtr->state & SRV_BUSY) == 0) {
	    Sync_MasterWait(&srvPtr->waitCondition,
				&srvPtr->mutex, TRUE);
	    if (sys_ShuttingDown) {
		srvPtr->state = SRV_NOTREADY;
		MASTER_UNLOCK(srvPtr->mutex);
		Sys_Printf("Rpc_Server exiting\n");
		Proc_Exit(0);
	    }
	}
	srvPtr->state &= ~SRV_NO_REPLY;
	MASTER_UNLOCK(srvPtr->mutex);

	/*
	 * At this point there is unsynchronized access to
	 * the server state.  We are marked BUSY, however, and
	 * RpcServerDispatch knows this and doesn't muck accordingly.
	 */

	/*
	 * Free up our previous reply.  The freeReplyProc is set by the
	 * call to Rpc_Reply.
	 */
	if ((Address)srvPtr->freeReplyProc != (Address)NIL) {
	    (void)(*srvPtr->freeReplyProc)(srvPtr->freeReplyData);
	    srvPtr->freeReplyProc = (int (*)())NIL;
	}

	rpcHdrPtr = &srvPtr->requestRpcHdr;
#ifdef TIMESTAMP
	RPC_TRACE(rpcHdrPtr, RPC_SERVER_A, " input");
#endif TIMESTAMP
	/*
	 * Monitor message traffic to keep track of other hosts.  This call
	 * has a side effect of blocking the server process while any
	 * crash recovery call-backs are in progress.
	 */
#ifndef NO_RECOVERY
	Recov_HostAlive(srvPtr->clientID, rpcHdrPtr->bootID,
			FALSE, rpcHdrPtr->flags & RPC_NOT_ACTIVE);
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
#endif TIMESTAMP
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
RpcReclaimServers()
{
    int srvIndex;
    register RpcServerState *srvPtr;
    int (*procPtr)();
    ClientData data;

    for (srvIndex=0 ; srvIndex < rpcNumServers ; srvIndex++) {
	srvPtr = rpcServerPtrPtr[srvIndex];

	MASTER_LOCK(srvPtr->mutex);


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
	    } else if ((srvPtr->state & SRV_AGING) == 0) {
		/*
		 * This is the first pass over the server process that
		 * has found it idle.  Start it aging.
		 */
		srvPtr->state |= SRV_AGING;
		srvPtr->age = 1;
	    } else {
		/*
		 * This process has aged since the last time we looked, at
		 * least sleepTime ago.  We resend to the client with the
		 * close flag and continue on.  If RpcServerDispatch gets
		 * a reply from the client closing its connection it will
		 * mark the server process SRV_FREE.  If we continue to
		 * send probes with no reply, we give up after N tries
		 * and free up the server.  It is possible that the client
		 * has re-allocated its channel, in which case it drops
		 * our probes on the floor.
		 */
		srvPtr->age++;
		if (srvPtr->age >= rpcMaxServerAge) {
		    procPtr = srvPtr->freeReplyProc;
		    data = srvPtr->freeReplyData;
		    srvPtr->freeReplyProc = (int (*)())NIL;
		    srvPtr->freeReplyData = (ClientData)NIL;
		    rpcSrvStat.reclaims++;
		    srvPtr->state = SRV_FREE;
		} else if (srvPtr->clientID == rpc_SpriteID) {
		    Sys_Panic(SYS_WARNING, "Reclaiming from myself.\n");
		    srvPtr->state = SRV_FREE;
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
	MASTER_UNLOCK(srvPtr->mutex);
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
void
RpcServerDispatch(srvPtr, rpcHdrPtr)
    register RpcServerState *srvPtr;	/* The state of the server process */
    register RpcHdr *rpcHdrPtr;		/* The header of the packet as it sits
					 * in the hardware buffers */
{
    register int size;		/* The amount of the data in the message */
    /*
     * Acquire the server's mutex for multiprocessor synchronization.  We
     * synchronize with each server process with a mutex that is part of
     * the server's state.
     */
    MASTER_LOCK(srvPtr->mutex);
    
#ifdef TIMESTAMP
    RPC_TRACE(rpcHdrPtr, RPC_SERVER_a, " server");
#endif TIMESTAMP
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
    if (rpcHdrPtr->ID != srvPtr->replyRpcHdr.ID) {

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
	     * This is an error by the client.  This server is working on
	     * a service request already and hasn't finished.
	     */
	    rpcSrvStat.serverBusy++;
	    goto unlock;
	}
	/*
	 * (There should be no bits set in the server's state word.)
	 *
	 * Update the server's idea of the current transaction.
	 */
	srvPtr->replyRpcHdr.ID = rpcHdrPtr->ID;
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
#endif TIMESTAMP
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
	    Sys_Printf("Unexpected Server state %x, idx %d, Rpc ID <%x> flags <%x> clientID %d\n",
			    srvPtr->index, srvPtr->state, rpcHdrPtr->ID,
			    rpcHdrPtr->flags, rpcHdrPtr->clientID);
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
			Sys_Printf("ServerDispatch - got a non-fragment\n");
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
#endif TIMESTAMP
    }
unlock:
    MASTER_UNLOCK(srvPtr->mutex);
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

    if (Net_IDToRoute(rpcHdrPtr->clientID) == (Net_Route *)NIL) {
	/*
	 * Make sure we have a good route back to the client.
	 */
	int mutex = 0;
	MASTER_LOCK(mutex);
	(void) Net_Arp(rpcHdrPtr->clientID, &mutex);
	MASTER_UNLOCK(mutex);
    }
    (void)RpcOutput(rpcHdrPtr->clientID, rpcHdrPtr, &srvPtr->reply,
					 (RpcBufferSet *)NIL, 0, (int *)NIL);
}


/*
 *----------------------------------------------------------------------
 *
 * RpcSrvInitHdr --
 *
 *	Initialize the header of a server message from fields
 *	in the clients request message.
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
    rpcHdrPtr->delay = rpcMyDelay;
    rpcHdrPtr->clientID = requestHdrPtr->clientID;
    rpcHdrPtr->serverID = rpc_SpriteID;
    rpcHdrPtr->channel = requestHdrPtr->channel;
    rpcHdrPtr->command = requestHdrPtr->command;
    rpcHdrPtr->serverHint = srvPtr->index;
    rpcHdrPtr->ID = requestHdrPtr->ID;
    rpcHdrPtr->bootID = rpcBootID;
    /*
     * This field should go away, but the UNIX file server depends on it now.
     */
    rpcHdrPtr->transport = PROTO_ETHER;

    rpcHdrPtr->numFrags = 0;
    rpcHdrPtr->fragMask = 0;
    rpcHdrPtr->paramSize = 0;
    rpcHdrPtr->dataSize = 0;
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
    int			(*freeReplyProc)();	/* Procedure to call to free
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
	 * Communicate the error code back in the command feild.
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

    if (Net_IDToRoute(rpcHdrPtr->clientID) == (Net_Route *)NIL) {
	/*
	 * Make sure we have a good route back to the client.
	 */
	int mutex = 0;
	MASTER_LOCK(mutex);
	(void) Net_Arp(rpcHdrPtr->clientID, &mutex);
	MASTER_UNLOCK(mutex);
    }
    (void)RpcOutput(rpcHdrPtr->clientID, rpcHdrPtr, &srvPtr->reply,
					 srvPtr->fragment, 0, (int *)NIL);
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
					 (RpcBufferSet *)NIL, 0, (int *)NIL);
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
     * Note, can't try ARP here because of it's synchronization with
     * a master lock and because we are called at interrupt time.
     */
    (void)RpcOutput(srvPtr->replyRpcHdr.clientID, &srvPtr->replyRpcHdr,
		       &srvPtr->reply, srvPtr->fragment,
		       srvPtr->fragsDelivered, (int *)NIL);
}

/*
 *----------------------------------------------------------------------
 *
 * RpcProbe --
 *
 *	Send a probe message to the client to see if it is still interested
 *	in the connection.  The Ack message header/buffer is used for this.
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
					 (RpcBufferSet *)NIL, 0,
					 &srvPtr->mutex);
}
