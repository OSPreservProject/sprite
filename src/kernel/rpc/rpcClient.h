/*
 * rpcClient.h --
 *
 *	Definitions for the client side of the RPC system.  The main object
 *	here is the Client Channel that keeps state during an RPC.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header: /cdrom/src/kernel/Cvsroot/kernel/rpc/rpcClient.h,v 9.9 92/12/13 18:21:31 mgbaker Exp $ SPRITE (Berkeley)
 */

#ifndef _RPCCLIENT
#define _RPCCLIENT

#include <netEther.h>
#ifdef KERNEL
#include <net.h>
#include <sync.h>
#include <timer.h>
#include <rpcTypes.h>
#include <rpcPacket.h>
#include <rpcCltStat.h>
#include <rpcHistogram.h>
#else
#include <kernel/net.h>
#include <kernel/sync.h>
#include <kernel/timer.h>
#include <kernel/rpcTypes.h>
#include <kernel/rpcPacket.h>
#include <kernel/rpcCltStat.h>
#include <kernel/rpcHistogram.h>
#endif /* KERNEL */


/*
 * Tunable parameters used by Rpc_Call.  They are packaged up here so
 * the values can be set/reported conveniently.
 */
typedef struct RpcConst {
    /*
     * The initial wait period is defined by retryWait, in ticks.  It
     * is initialized from the millisecond valued retryMsec.  This is
     * how long we wait after sending the first request packet.  If
     * we are sending a fragmented packet we have a longer retry period.
     */
    int  		retryMsec;
    unsigned int 	retryWait;
    int  		fragRetryMsec;
    unsigned int	fragRetryWait;
    /*
     * The wait period increases if we have to resend.  If we are recieving
     * acknowledgments then we increase the timout until maxAckWait (ticks),
     * which is initialized from maxAckMsec.  If we are not getting acks
     * then we still back off a bit (as a heuristic in case  we are talking
     * to a slow host) until the timout period is maxTimeoutWait, which
     * is initialized from maxTimeoutMsec.
     */
    int  		maxAckMsec;
    unsigned int	maxAckWait;
    int  		maxTimeoutMsec;
    unsigned int	maxTimeoutWait;
    /*
     * maxTries is the maximum number of times we resend a request
     * without getting any response (i.e. acknowledgments or the reply).
     * maxAcks is the maximum number of acknowledgments we'll receive
     * before complaining "so and so seems hung".  In the case of a
     * broadcast rpc, we'll abort the RPC after maxAcks acknowledgments.
     */
    int 		maxTries;
    int 		maxAcks;
} RpcConst;

/*
 * Two sets of timeouts are defined, one for local ethernet, one for IP.
 */
extern RpcConst rpcEtherConst;
extern RpcConst rpcInetConst;


/*
 * Global used by Rpc_Call and initialized by Rpc_Start.  This is set
 * once at boot-time to the real time clock.  The servers track this value
 * and detects a reboot by us when this changes.  Note that if the boot
 * ID is zero then the server doesn't attempt to detect a reboot.  Thus
 * we can use a GET_TIME RPC in order to set this.  See Rpc_Start.
 */
extern unsigned int rpcBootId;


/*
 *      An RPC channel is described here.  It is used during an RPC to
 *      keep the state of the RPC.  Between uses the channel carries over
 *      some information to use as hints in subsequent transactions.
 */
typedef struct RpcClientChannel {
    /*
     * The Channel Index is a self reference to this channel.
     * It is this channels index in the array of channel pointers.
     * It is kept here because it will be part of the packet header.
     */
    int			index;
    /*
     * Rpc transaction state:  There are state bits to drive the algorithm,
     * the values are described below with their definitions.
     */
    int			state;
    /*
     * The ID of the server the channel is being used with.
     * A value of -1 means the channel has not been used yet.
     */
    int			serverID;
    /*
     * These timeout parameters depend on the server being used.
     */
    RpcConst		*constPtr;
    /*
     * A channel may wait on input, and be taken out of the timeout queue
     * at interrupt time.  The timer queue element is stored here so
     * it is accessable by the interrupt time routine.
     */
    Timer_QueueElement timeoutItem;
    /*
     * RpcDoCall and RpcClientDispatch synchronize with a master lock.
     * The interrupt level routine notifies waitCondition when the waiting
     * process should wakeup and check for input.
     */
    Sync_Semaphore	mutex;
    Sync_Condition	waitCondition;
    /*
     * This bitmask indicates which fragments in the current
     * request message the server has received.
     */
    int			fragsDelivered;
    /*
     * This bitmask indicates which fragments in the current reply
     * message we have received.
     */
    int			fragsReceived;
    /*
     * Two temporaries are needed to record the
     * amount of data actually returned by the server.
     */
    int			actualDataSize;
    int			actualParamSize;
    /*
     * The header and buffer specifications for the request message.
     */
    RpcHdr		requestRpcHdr;
    RpcBufferSet	request;
    /*
     * An array of RPC headers and buffer sets that are used when fragmenting
     * a request message.
     */
    RpcHdr		fragRpcHdr[RPC_MAX_NUM_FRAGS];
    RpcBufferSet	fragment[RPC_MAX_NUM_FRAGS];

    /*
     * Header and buffer specification for the reply message.
     */
    RpcHdr		replyRpcHdr;
    RpcBufferSet	reply;
    /*
     * The header and buffer specifications for the explicit acknowledgments.
     */
    RpcHdr		ackHdr;
    RpcBufferSet	ack;
} RpcClientChannel;

/*
 * Definitions of state bits for a remote procedure call.
 *  CHAN_FREE		The channel is free.
 *  CHAN_BUSY		The channel is in use.
 *  CHAN_WAITING	The channel available for input.
 *  CHAN_INPUT		The channel has received input.
 *  CHAN_TIMEOUT	The channel is in the timeout queue.
 *  CHAN_FRAGMENTING	The channel is awaiting fragment reassembly.
 */
#define CHAN_FREE		0x40
#define CHAN_BUSY		0x01
#define CHAN_WAITING		0x02
#define CHAN_INPUT		0x04
#define CHAN_TIMEOUT		0x08
#define CHAN_FRAGMENTING	0x10

/*
 * The set of channels is dynamically allocated at boot time and
 * they are referenced through an array of pointers.
 */
extern RpcClientChannel **rpcChannelPtrPtr;
extern int		  rpcNumChannels;

/*
 * These are variables to control handling of negative acknowledgement
 * back-off on the clients.  Maybe they should be in the client channel
 * state?  But there's no server state associated with the negative ack,
 * so this wouldn't necessarily make sense.
 */
extern	int	rpcNackRetryWait;
extern	int	rpcMaxNackWait;

/* Whether or not client ramps down channels in response to a neg. ack. */
extern	Boolean	rpcChannelNegAcks;

/*
 * A histogram of the call times for the different RPCs.
 */
extern Rpc_Histogram	*rpcCallTime[];

/*
 * A raw count of the number of rpc calls.
 */
extern int		rpcClientCalls[];

/*
 * Forward declarations.
 */
extern void RpcChanFree _ARGS_((RpcClientChannel *chanPtr));
extern void RpcChanClose _ARGS_((register RpcClientChannel *chanPtr, register RpcHdr *rpcHdrPtr));
extern void RpcSetup _ARGS_((int serverID, int command, register Rpc_Storage *storagePtr, register RpcClientChannel *chanPtr));
extern RpcClientChannel *RpcChanAlloc _ARGS_((int serverID));
extern ReturnStatus RpcDoCall _ARGS_((int serverID, register RpcClientChannel *chanPtr, Rpc_Storage *storagePtr, int command, unsigned int *srvBootIDPtr, int *notActivePtr, unsigned int *recovTypePtr));
extern void RpcClientDispatch _ARGS_((register RpcClientChannel *chanPtr, register RpcHdr *rpcHdrPtr));
extern void RpcInitServerChannelState _ARGS_((void));


#endif /* _RPCCLIENT */
