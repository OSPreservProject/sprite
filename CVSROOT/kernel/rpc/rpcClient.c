/*
 * rpcClient.c --
 *
 *      The client side of the RPC protocol.  The routines here are in
 *      this file because they synchronize with each other using a master
 *      lock.  RpcDoCall is the send-receive-timeout loop and
 *      RpcClientDispatch is the interrupt time routine that gets packets
 *      and hands them up to RpcDoCall.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */


#include <sprite.h>
#include <stdio.h>
#include <rpc.h>
#include <rpcInt.h>
#include <rpcClient.h>
#include <rpcServer.h>
#include <rpcTrace.h>
#include <dbg.h>
#include <proc.h>
#include <sys.h>


/*
 * For debugging servers.  We allow client's to retry forever instead
 * of timing out.  This is exported and settable via Fs_Command
 */
Boolean rpc_NoTimeouts = FALSE;

/*
 * A histogram is kept of the elapsed time of each different kind of RPC.
 */
Rpc_Histogram *rpcCallTime[RPC_LAST_COMMAND+1];
Boolean rpcCallTiming = FALSE;

#ifdef DEBUG
#define DEBUGSIZE 1000
#define INC(ctr) { (ctr) = ((ctr) == DEBUGSIZE-1) ? 0 : (ctr)+1; }
typedef struct {
    RpcClientChannel	*chanPtr;
    char		*action;
    int			pNum;
    int			serverID;
    int			chanNum;
    int			state;
} dbgElem;

static dbgElem	dbgArray[DEBUGSIZE];
static int 		dbgCtr;

#define CHAN_TRACE(zchanPtr, serverID, string) \
{ \
	dbgElem *ptr = &dbgArray[dbgCtr]; \
	INC(dbgCtr); \
	ptr->chanPtr = zchanPtr; \
	ptr->action = string; \
	ptr->chanNum = zchanPtr->index; \
	ptr->serverID = serverID; \
	ptr->pNum = Mach_GetProcessorNumber(); \
	ptr->state = zchanPtr->state; \
}

#else
#define CHAN_TRACE(zchanPtr, serverID, string)
#endif

/* Variables to control nack back-off on client. */
int	rpcNackRetryWait;
int	rpcMaxNackWait;
/*
 * This variable determines whether we use client policy of ramping down
 * channels for neg acks.  The default is to use backoff.
 */
Boolean	rpcChannelNegAcks = FALSE;


/*
 *----------------------------------------------------------------------
 *
 * RpcDoCall --
 *
 *	The send-receive-timeout loop on the client for a remote procedure call.
 *
 * Results:
 *	The return code from the remote procedure or an error code
 *	related to the RPC protocol, or SUCCESS.
 *
 * Side effects:
 *	The remote procedure call itself.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
RpcDoCall(serverID, chanPtr, storagePtr, command, srvBootIDPtr, notActivePtr)
    int serverID;		/* The Sprite host that will execute the
				 * service procedure */
    register RpcClientChannel *chanPtr;	/* The channel for the RPC */
    Rpc_Storage *storagePtr;	/* Pointers to caller's buffers */
    int command;		/* Only used to filter trace records */
    unsigned	int *srvBootIDPtr;	/* Return, boot time stamp of server. */
    int *notActivePtr;		/* Return, RPC_NOT_ACTIVE flag from server.
				 * These last two return parameters are later
				 * passed to the recovery module. */
{
    register RpcHdr *rpcHdrPtr;	/* Pointer to received message header */
    register RpcConst *constPtr;/* Timeout parameter block */
    register ReturnStatus error;/* General error return status */
    register unsigned int wait;	/* Wait interval for timeouts */
    int numAcks;		/* Count of acks received.  Used to catch the
				 * case where the server process hangs and
				 * the server dispatcher acks us forever */
    register int numTries;	/* Number of times we sent the message while
				 * getting no reply. */
    register unsigned int lastFragMask = 0;	/* Previous state of our
						 * fragment reassembly */
    Boolean	seemsHung = FALSE;	/* Used to control warning msgs */

    /*
     * This code is locked with MASTER_LOCK in order to synchronize
     * with the RpcClientDispatch routine.  Inside the critical
     * section we sleep on the channel's wait condition to which
     * RpcClientDispatch broadcasts when it gets input.  Furthermore,
     * we place a call back procedure in the timer queue that will
     * also notify that condition upon timeout.
     */
    MASTER_LOCK(&chanPtr->mutex);

    /*
     * Send the request off to the server.  We update the server hint from
     * the channel's return message header.  ie. take the last server
     * hint received from the server.
     */

    *srvBootIDPtr = 0;
    rpcCltStat.requests++;
    chanPtr->requestRpcHdr.serverHint =	chanPtr->replyRpcHdr.serverHint;
    chanPtr->state |= CHAN_WAITING;
    error = RpcOutput(serverID, (RpcHdr *) &chanPtr->requestRpcHdr,
		      &chanPtr->request, chanPtr->fragment,
		      (unsigned int) (chanPtr->fragsDelivered),
		      &chanPtr->mutex);
    /*
     * Set up the initial wait interval.  The wait could depend on
     * characteristics of the RPC, or of the other host.
     * For now we just wait longer if the packet will be fragmented.
     */
    constPtr = chanPtr->constPtr;
    if ((storagePtr->requestDataSize + storagePtr->requestParamSize >
	    RPC_MAX_FRAG_SIZE) ||
	(storagePtr->replyDataSize + storagePtr->replyParamSize >
	    RPC_MAX_FRAG_SIZE)) {
	wait = constPtr->fragRetryWait;
    } else {
	wait = constPtr->retryWait;
    }

    /*
     * Loop waiting for input and re-sending if need be.  As well as
     * getting a reply from the server we screen out junk and react to
     * explicit acks.  This times out after a number of times around the
     * loop with no input.
     */
    numTries = 0;
    numAcks = 0;
    do {

	/*
	 * Wait until we get a poke from the timeout routine or there
	 * is input available.  Input may have arrived before we get
	 * here because the channel mutex is released while Rpc_Output
	 * waits for the packet to be sent by the network interface.
	 */
	if (! (chanPtr->state & CHAN_INPUT)) {
	    chanPtr->timeoutItem.routine = Rpc_Timeout;
	    chanPtr->timeoutItem.interval = wait;
	    chanPtr->timeoutItem.clientData = (ClientData)chanPtr;
	    chanPtr->state |= CHAN_TIMEOUT | CHAN_WAITING;
	    CHAN_TRACE(chanPtr, serverID, "about to schedule");
	    Timer_ScheduleRoutine(&chanPtr->timeoutItem, TRUE);
	    do {
		/*
		 * Wait ignoring signals.
		 */
		Sync_MasterWait(&chanPtr->waitCondition,
				&chanPtr->mutex, FALSE);
	    } while (((chanPtr->state & CHAN_INPUT) == 0) &&
		     (chanPtr->state & CHAN_TIMEOUT));
	    CHAN_TRACE(chanPtr, serverID, "woken up");
	}
	if (chanPtr->state & CHAN_INPUT) {
	    /*
	     * Got some input.  The dispatch routine has copied the
	     * packet into the areas refered to by the reply BufferSet.
	     *
	     * NB: We have to completely process this message before we
	     * can accept another message on this channel.  There is no
	     * mechanism to queue messages.
	     */
	    chanPtr->state &= ~CHAN_INPUT;
	    rpcHdrPtr = &chanPtr->replyRpcHdr;

	    /*
	     * Pick off the boot timestamp and active state of the server so
	     * the recovery module can pay attention to traffic.
	     */
	    *notActivePtr = rpcHdrPtr->flags & RPC_NOT_ACTIVE;
	    *srvBootIDPtr = rpcHdrPtr->bootID;
#ifdef TIMESTAMP
	    RPC_TRACE(rpcHdrPtr, RPC_CLIENT_D, "input");
#endif /* TIMESTAMP */
	    if (rpcHdrPtr->ID != chanPtr->requestRpcHdr.ID) {
		/*
		 * Note old message.
		 */
		rpcCltStat.oldInputs++;

	    } else if (rpcHdrPtr->flags & RPC_NACK) {
		/*
		 * NOTE: for now we must handle a NACK before an ACK because
		 * I'm OR'ing in an ACK with the NACK so that old kernels on
		 * clients won't freak if they receive a NACK.  This backwards
		 * compatibility should be removed later.  The changes that go
		 * with it are setting the serverHint in RpcSrvInitHdr() and
		 * the OR'ing itself of the ACK with the NACK in rpcServer.c.
		 */
		rpcCltStat.nacks++;
		/*
		 * Try out different nack-handling policies.  
		 * We can either back off as in an ACK, or try to ramp
		 * down the number of channels.
		 */
		if (serverID != RPC_BROADCAST_SERVER_ID) {
		    numTries = 0;
		    if (!rpcChannelNegAcks) {
			if (wait < rpcNackRetryWait) {
			    wait = rpcNackRetryWait;
			} else {
			    Net_HostPrint(serverID,
			    "Client backing off again from negative ack.\n");
			    wait *= 2;
			    rpcCltStat.reNacks++;
			}
			if (wait > rpcMaxNackWait) {
			    Net_HostPrint(serverID,
			    "Client setting max backoff from negative ack.\n");
			    wait = rpcMaxNackWait;
			    rpcCltStat.maxNacks++;
			}
		    } else {
			/* Return error to cause ramping down of channels. */
			error = RPC_NACK_ERROR; 
		    }
		}
	    } else if (rpcHdrPtr->flags & RPC_REPLY) {
		/*
		 * Our reply, check for an error code and break from the
		 * receive loop.  The command field is overloaded with the
		 * return error code.
		 */
		if (rpcHdrPtr->flags & RPC_ERROR) {
		    error = (ReturnStatus)rpcHdrPtr->command;
		    if (error == 0) {
			rpcCltStat.nullErrors++;
			error = RPC_NULL_ERROR;
		    } else {
			rpcCltStat.errors++;
		    }
		} else {
		    rpcCltStat.replies++;
		}
		/*
		 * Copy back the return buffer size (it's set by ClientDispatch)
		 * to reflect what really came back.
		 */
		storagePtr->replyDataSize = chanPtr->actualDataSize;
		storagePtr->replyParamSize = chanPtr->actualParamSize;
		break;
	    } else if (rpcHdrPtr->flags & RPC_ACK) {
		numAcks++;
		rpcCltStat.acks++;
		if (numAcks <= constPtr->maxAcks) {
		    /*
		     * An ack from the server indicating that a server
		     * process is working on our request.  We increase
		     * our waiting time to decrease the change that we'll
		     * timeout again before receiving the reply.
		     * NOTE: We don't pay attention to acks if we are
		     * broadcasting.  This makes the broadcaster too vulnerable
		     * to errant servers.  In particular, diskless clients
		     * often wedge trying to handle a prefix request, send
		     * the real server a lot of acks, and slow it's boot down.
		     */
		    if (serverID != RPC_BROADCAST_SERVER_ID) {
			numTries = 0;
			wait *= 2;
			if (wait > constPtr->maxAckWait) {
			    wait = constPtr->maxAckWait;
			}
		    }
		} else {
		    char name[100];
		    /*
		     * Too many acks.  It is very likely that the server
		     * process is hung on some lock.  We hang too in
		     * order to facilitate debugging.
		     */
		    rpcCltStat.tooManyAcks++;
		    if (!seemsHung) {
			Net_SpriteIDToName(serverID, 100, name);
			if (name == (char *)NIL) {
			    printf("RpcDoCall: <%s> RPC to host <%d> is hung\n",
				rpcService[command].name, serverID);
			} else {
			    printf("RpcDoCall: <%s> RPC to %s is hung\n",
				rpcService[command].name, name);
			}
			seemsHung = TRUE;
		    }
		    numAcks = 0;
		}
	    } else {
		/*
		 * Unexpected kind of input
		 */
		rpcCltStat.badInput++;
		printf("Warning: Rpc_Call: Unexpected input.\n");
		error = RPC_INTERNAL_ERROR;
	    }
	} else {
	    /*
	     * Have not received a complete message yet.  Update the
	     * server hint and then re-send the request or send a partial
	     * acknowledgment.
	     */
	    chanPtr->requestRpcHdr.serverHint =
		chanPtr->replyRpcHdr.serverHint;
	    rpcCltStat.timeouts++;
	    /*
	     * Back off upon timeout because we may be talking to a slow host
	     */
	    wait *= 2;
	    if (wait > constPtr->maxTimeoutWait) {
		wait = constPtr->maxTimeoutWait;
	    }
	    if ((chanPtr->state & CHAN_FRAGMENTING) == 0) {
		/*
		 * Not receiving fragments.  Check for timeout, and resend
		 * the request.
		 */
		numTries++;
		if (numTries < constPtr->maxTries ||
		    (rpc_NoTimeouts && serverID != RPC_BROADCAST_SERVER_ID)) {
		    rpcCltStat.resends++;
		    chanPtr->requestRpcHdr.flags |= RPC_PLSACK;
		    error = RpcOutput(serverID, 
				      (RpcHdr *) &chanPtr->requestRpcHdr,
				      &chanPtr->request, chanPtr->fragment,
				      (unsigned int) (chanPtr->fragsDelivered),
				      &chanPtr->mutex);
		} else {
		    rpcCltStat.aborts++;
		    error = RPC_TIMEOUT;
		}
	    } else {
		/*
		 * We are getting a fragmented response.  The client dispatcher
		 * has set the fragsReceived field to reflect the state
		 * of fragment reassembly.  We check that and will abort
		 * if we timeout too many times with no new fragments.
		 * Otherwise we return a partial acknowledgment.
		 */
		if (lastFragMask < chanPtr->fragsReceived) {
		    lastFragMask = chanPtr->fragsReceived;
		    numTries = 0;
		} else {
		    numTries++;
		}
		if (numTries >= constPtr->maxTries &&
		    (!rpc_NoTimeouts||(serverID == RPC_BROADCAST_SERVER_ID))) {
		    rpcCltStat.aborts++;
		    error = RPC_TIMEOUT;
		} else {
		    chanPtr->requestRpcHdr.flags = RPC_SERVER |RPC_ACK |
						    RPC_LASTFRAG;
		    chanPtr->requestRpcHdr.fragMask = chanPtr->fragsReceived;
		    rpcCltStat.sentPartial++;
		    error = RpcOutput(serverID, 
				      (RpcHdr *) &chanPtr->requestRpcHdr,
				      &chanPtr->request, chanPtr->fragment,
				      (unsigned int) (chanPtr->fragsDelivered),
				      &chanPtr->mutex);
		}
	    }
	}
    } while (error == SUCCESS);
    chanPtr->state &= ~CHAN_WAITING;
    if (seemsHung) {
	if (error == SUCCESS) {
	    printf("<%s> RPC ok\n", rpcService[command].name);
	} else {
	    printf("<%s> RPC exit 0x%x\n", rpcService[command].name, error);
	}
    }
    MASTER_UNLOCK(&chanPtr->mutex);
    return(error);
}

/*
 *----------------------------------------------------------------------
 *
 * RpcClientDispatch --
 *
 *      Dispatch a message to a client channel.  The channel has buffer
 *      space for the packet header and also specifies the buffer areas
 *      for the RPC return parameters and data.  We notify the owner of
 *      the channel that is has input via the condition variable in the
 *      channel.
 *
 * Sprite Id:
 *	This routine has the side effect of initializing the Sprite ID
 *	of the host from information in the RPC packet header.  This is
 *	for diskless clients of the filesystem that have no other way
 *	to determine their Sprite ID.  The routine RpcValidateClient
 *	on the server side initializes the clientID field for us.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The message is copied into the buffers specified by the channel
 *	and the channel is notified of input.
 *
 *----------------------------------------------------------------------
 */
void
RpcClientDispatch(chanPtr, rpcHdrPtr)
    register RpcClientChannel *chanPtr;	/* The channel the packet is for */
    register RpcHdr *rpcHdrPtr;		/* The Rpc header as it sits in the
					 * network module's buffer.  The data
					 * in the message follows this. */
{
    register int size;	/* The amount of data in the message */
    /*
     * Acquire the channel mutex for multiprocessor synchronization.
     * Deadlock occurs here when doing RPCs to oneself and the client
     * resends and then the server acknowledges.
     */
#if (MACH_MAX_NUM_PROCESSORS == 1) /* uniprocessor implementation */
    if (chanPtr->mutex.value != 0) {
	printf("Warning:  Rpc to myself?\n");
	return;
    } else {
	MASTER_LOCK(&chanPtr->mutex);
    }
#else	/* Multiprocessor implementation. */
    MASTER_LOCK(&chanPtr->mutex);
#endif
    
    /*
     * Discover our own Sprite ID from the packet header.
     */
    if (rpc_SpriteID == 0) {
	rpc_SpriteID = rpcHdrPtr->clientID;
	printf("RPC setting SpriteID to %d.\n", rpc_SpriteID);
    } else if (rpc_SpriteID != rpcHdrPtr->clientID) {
	printf("RpcClientDispatch: clientID changed from (%d) to (%d).\n",
				       rpc_SpriteID, rpcHdrPtr->clientID);
    }

    /*
     * See if this is a close request from the server - the server host is
     * trying to recycle the server process that is bound to this
     * channel.  If the channel is not busy now it implies we have completed
     * the RPC the server is inquiring about.  We return an ack to the
     * server and unbind the server process from us.  During this the
     * channel is marked busy so that while we are returning the ack,
     * another process doesn't come along, allocate the channel, and try
     * to use the same buffers as us.
     */
    if (rpcHdrPtr->flags & RPC_CLOSE) {
	RpcChanClose(chanPtr, rpcHdrPtr);
	goto unlock;
    }

    /*
     * Verify the transaction Id.  Doing this now after checking for
     * close messages means we still ack close requests even if
     * we have already recycled the channel.
     */
    if (rpcHdrPtr->ID != chanPtr->requestRpcHdr.ID) {
	rpcCltStat.badId++;
	goto unlock;
    }

    /*
     * Filter out partial acks.
     */
    if ((rpcHdrPtr->fragMask != 0) && (rpcHdrPtr->flags & RPC_ACK)) {
	if (chanPtr->fragsDelivered != rpcHdrPtr->fragMask) {
	    /*
	     * Because we may get several partial acks, we just update
	     * fragsDelivered so the next resend is smarter.  Eventually
	     * we'll get all the fragments delivered and then these
	     * acks will get passed up to RpcDoCall().
	     */
	    chanPtr->fragsDelivered = rpcHdrPtr->fragMask;
	    rpcCltStat.recvPartial++;
	    goto unlock;
	}
	/*
	 * Apparently we've gotten everything through to the
	 * server.  Our last transmission was a keep-alive
	 * of just the last fragment.  We FALL THROUGH to
	 * pass the server's ack up to the process level, RpcDoCall();
	 */
    }

    /*
     * See if the channel is available for input.  Currently there
     * is no queueing of packets so we drop this packet if the process
     * is still working on the last packet it got.
     */
    if ((chanPtr->state & CHAN_WAITING) == 0) {
	rpcCltStat.chanBusy++;
	goto unlock;
    }
    /*
     * Note for RpcDoCall the actual size of the returned information.
     */
    size = rpcHdrPtr->paramSize + rpcHdrPtr->paramOffset;
    if (chanPtr->actualParamSize < size) {
	chanPtr->actualParamSize = size;
    }
    size = rpcHdrPtr->dataSize + rpcHdrPtr->dataOffset;
    if (chanPtr->actualDataSize < size) {
	chanPtr->actualDataSize = size;
    }

    if (rpcHdrPtr->numFrags != 0) {
	if ((chanPtr->state & CHAN_FRAGMENTING) == 0) {
	    /*
	     * The first fragment of a fragmented reply.
	     */
	    RpcScatter(rpcHdrPtr, &chanPtr->reply);
	    chanPtr->fragsReceived = rpcHdrPtr->fragMask;
	    chanPtr->state |= CHAN_FRAGMENTING;
	    goto unlock;
	} else if (chanPtr->fragsReceived & rpcHdrPtr->fragMask) {
	    /*
	     * Duplicate fragment.
	     */
	    rpcCltStat.dupFrag++;
	    goto unlock;
	} else {
	    /*
	     * More fragments.
	     */
	    RpcScatter(rpcHdrPtr, &chanPtr->reply);
	    chanPtr->fragsReceived |= rpcHdrPtr->fragMask;
	    if (chanPtr->fragsReceived !=
	       rpcCompleteMask[rpcHdrPtr->numFrags]) {
		goto unlock;
	    } else {
		/*
		 * Now the packet is complete.
		 */
		chanPtr->state &= ~CHAN_FRAGMENTING;
	    }
	}
    } else {
	/*
	 * Unfragmented message.
	 * Copy the complete message out of the network's buffers.
	 */
	RpcScatter(rpcHdrPtr, &chanPtr->reply);
    }
	
    /*
     * Remove the channel from the timeout queue and
     * notify the waiting channel that it has input.
     */
    chanPtr->state &= ~CHAN_WAITING;
    chanPtr->state |= CHAN_INPUT;

    if (chanPtr->state & CHAN_TIMEOUT) {
	chanPtr->state &= ~CHAN_TIMEOUT;
	CHAN_TRACE(chanPtr, chanPtr->serverID, "about to deschedule");
	(void)Timer_DescheduleRoutine(&chanPtr->timeoutItem);
    }
    Sync_MasterBroadcast(&chanPtr->waitCondition);

unlock:
#ifdef TIMESTAMP
    RPC_TRACE(rpcHdrPtr, RPC_CLIENT_a, "client");
#endif /* TIMESTAMP */

    MASTER_UNLOCK(&chanPtr->mutex);
}

/*
 *----------------------------------------------------------------------
 *
 * Rpc_Timeout --
 *
 *	Called when a channel times out.  This notifies the waiting condition
 *	of the channel so the process can wake up and take action upon
 *	the timeout.  The mutex on the channel is aquired for multiprocessor
 *	synchronization.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The channel contition is notified.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
void
Rpc_Timeout(time, data)
    Timer_Ticks time;	/* The time we timed out at. */
    ClientData data;	/* Our private data is the channel pointer */
{
    RpcClientChannel *chanPtr;	/* The channel to notify */

    chanPtr = (RpcClientChannel *)data;

    /*
     * This is called from the timeout queue at interrupt time.
     * We acquire the master lock (as a formality in a uniprocessor)
     * and use the form of broadcast designed for master locks.
     */
    MASTER_LOCK(&chanPtr->mutex);
    chanPtr->state &= ~CHAN_TIMEOUT;
    CHAN_TRACE(chanPtr, chanPtr->serverID, "Timeout");
    Sync_MasterBroadcast(&chanPtr->waitCondition);
    MASTER_UNLOCK(&chanPtr->mutex);
}
