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
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _RPCCLIENT
#define _RPCCLIENT

#include "netEther.h"
#include "net.h"
#include "sync.h"
#include "timer.h"

#include "rpcInt.h"
#include "rpcCltStat.h"
#include "rpcHistogram.h"

/*
 * Tunable parameters used by Rpc_Call.  They are packaged up here so
 * the values can be set/reported conveniently.
 */
typedef struct RpcConst {
    /*
     * The initial wait period is defined by retryWait, in ticks.  It
     * is initialized from the millisecond valued retryMsec.  This is
     * how long we wait after sending the first request packet.
     */
    int  		retryMsec;
    unsigned int 	retryWait;
    /*
     * The wait period increases if we have to resend.  If we are recieving
     * acknowledgments then we increase the timout until maxAckWait (ticks),
     * which is initialized from maxAckMsec.  If we are not getting acks
     * then we still back off a bit (as a heuristic in case  we are talking
     * to a slow host) until the timout periodis maxTimeoutWait, which
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

extern RpcConst rpc;
extern int rpcRetryMsec;
extern int rpcMaxAckMsec;
extern int rpcMaxTimeoutMsec;
extern int rpcMaxTries;
extern int rpcMaxAcks;

/*
 * Global used by Rpc_Call and initialized by Rpc_Start.  This is set
 * once at boot-time to the real time clock.  The server tracks this value
 * and detects a reboot by us when this changes.
 */
extern unsigned int rpcBootId;


/*
 *      An RPC channel is described here.  It is used during an RPC to
 *      keep the state of the RPC.  Between uses the channel carries over
 *      some information to use as hints in subsequent transactions.
 */
typedef struct RpcClientChannel {
    /*
     * The ID of the server the channel is being used with.
     * A value of -1 means the channel has not been used yet.
     */
    int			serverID;
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
     * Two temporaries are needed to record the
     * amount of data actually returned by the server.
     */
    int			actualDataSize;
    int			actualParamSize;

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
    int			mutex;
    Sync_Condition	waitCondition;

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
#define CHAN_FREE	0x00
#define CHAN_BUSY	0x01
#define CHAN_WAITING	0x02
#define CHAN_INPUT	0x04
#define CHAN_TIMEOUT	0x08
#define CHAN_FRAGMENTING	0x10

/*
 * The set of channels is dynamically allocated at boot time and
 * they are referenced through an array of pointers.
 */
extern RpcClientChannel **rpcChannelPtrPtr;
extern int		  rpcNumChannels;

/*
 * A histogram of the call times for the different RPCs.
 */
extern Rpc_Histogram	*rpcCallTime[];

/*
 * Forward declarations.
 */
RpcClientChannel	*RpcChanAlloc();
void			 RpcChanFree();
void			 RpcSetup();
ReturnStatus		 RpcDoCall();
void			 RpcClientDispatch();

#endif /* _RPCCLIENT */
