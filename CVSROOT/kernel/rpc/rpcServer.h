/*
 * rpcServer.h --
 *
 *	Definitions for the server side of the RPC system.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _RPCSERVER
#define _RPCSERVER

#ifdef KERNEL
#include <net.h>
#include <sync.h>
#include <timer.h>
#include <rpcTypes.h>
#include <rpcPacket.h>
#include <rpcSrvStat.h>
#include <rpcHistogram.h>
#else
#include <kernel/net.h>
#include <kernel/sync.h>
#include <kernel/timer.h>
#include <kernel/rpcTypes.h>
#include <kernel/rpcPacket.h>
#include <kernel/rpcSrvStat.h>
#include <kernel/rpcHistogram.h>
#endif /* KERNEL */


/*
 *      Definition of the state table maintained for all the RPC server
 *      processes.  The state of each server is similar to the state kept
 *      for each client in the channel table.  It includes the current RPC
 *      sequence number, the client's address and channel number, and
 *      buffer space for input and reply message headers.  The server
 *      state table is scanned during dispatch in order to find the server
 *      for incoming messages.
 */
typedef struct RpcServerState {
    /*
     * Rpc transaction state:  There are state bits to drive the algorithm,
     * the values are described below with their definitions.
     */
    int			state;
    /*
     * The current RPC sequence number.
     */
    unsigned int	ID;
    /*
     * The index is a self reference to this server's state.
     * It is kept here because it will be part of the packet header.
     */
    int			index;
    /*
     * Servers go into an aging state after waiting for a reply
     * from the client for too long.  A callback procedure is put
     * in the the timer queue to probe the client, and the server's
     * age is recorded here.
     */
    int			age;
    Timer_QueueElement timeoutItem;

    /*
     * The client's host ID and channel number uniquely identify it.
     * This is used to match incoming requests to the correct server process.
     */
    int			clientID;
    int			channel;

    /*
     * Synchronization between the dispatcher and the server processes
     * is with 'mutex', a master lock.  The condition variable is
     * used to notify the server process that it has input.
     */
    Sync_Semaphore	mutex;
    Sync_Condition	waitCondition;

    /*
     * This bitmask indicates which fragments our client has recieved.
     * It is used by RpcOutput for partial resends.
     */
    unsigned	int	fragsDelivered;
    /*
     * This bitmask indicates which fragments we've gotten.
     * It is maintained by ServerDispatch and returned to the
     * client in partial acknowledgments.
     */
    int			fragsReceived;

    /*
     * Header and buffer specifications for request messages.
     */
    RpcHdr		requestRpcHdr;
    RpcBufferSet	request;

    /*
     * Header and buffer specification for the reply message.
     */
    RpcHdr		replyRpcHdr;
    RpcBufferSet	reply;

    /*
     * A callback procedure is used to free up the reply.  This is called
     * by the RPC module when it knows the client has successfully received
     * the reply, or it has crashed.
     */
    int			(*freeReplyProc) _ARGS_((ClientData freeReplyData));
    ClientData		freeReplyData;

    /*
     * An array of RPC headers and buffer specifications is needed
     * when fragmenting a large reply.
     */
    RpcHdr		fragRpcHdr[RPC_MAX_NUM_FRAGS];
    RpcBufferSet	fragment[RPC_MAX_NUM_FRAGS];

    /*
     * Buffer space for server acknowlegment messages.
     */
    RpcHdr		ackRpcHdr;
    RpcBufferSet	ack;

    /*
     * Two temporaries are needed to record the
     * amount of data actually sent by the client.
     */
    int			actualDataSize;
    int			actualParamSize;

} RpcServerState;

/*
 * These are data structures for sending negative acknowledgements when
 * there's no server available to allocate to a client channel.  Since there's
 * no server available, this stuff isn't part of the server state.
 */
typedef struct  NackData {
    RpcHdr              *rpcHdrArray;		/* headers to transmit */
    int			*hdrState;		/* which headers are free */
    RpcBufferSet        *bufferSet;		/* buffers for transmission */
    Sync_Semaphore      mutex;			/* protect nack data */
    int			numFree;		/* are any free? */
} NackData;
extern	NackData	rpcNack;
extern	int		rpc_NumNackBuffers;	/* settable number of buffers */
#define	RPC_NACK_FREE		0		/* Can use this header */
#define	RPC_NACK_WAITING	1		/* Hdr full, waiting for xmit */
#define	RPC_NACK_XMITTING	2		/* Hdr being xmitted */


/*
 * Definitions of state bits for a remote procedure call.
 *  SRV_NOTREADY	The server has no buffer space yet.
 *  SRV_FREE		The server is free.
 *  SRV_BUSY		The server is working on a request.
 *  SRV_WAITING		The server has sent off its reply.
 *  SRV_AGING		The server is aging, waiting for a client request.
 *  SRV_FRAGMENT	 ... waiting for reasembly of a fragmented request.
 *  SRV_NO_REPLY	The server is explicitly not returning a reply
 *			in response to a broadcast request.  This state
 *			prevents the dispatcher from sending acknowledments
 *			and prevents the top level server process from
 *			sending a default error reply to the client.
 * SRV_STUCK		This server is apparently stuck during an RPC for
 *			a dead or rebooted client.  The server will not
 *			be picked by the dispatcher until its current
 *			RPC completes and this flag is reset.
 */
#define SRV_NOTREADY	0x00
#define SRV_FREE	0x01
#define SRV_BUSY	0x02
#define SRV_WAITING	0x04
#define SRV_AGING	0x08
#define SRV_FRAGMENT	0x10
#define SRV_NO_REPLY	0x20
#define SRV_STUCK	0x40

/*
 * The server's state table has a maximum number of entries, but not all
 * the entries are initialized and have an associated process.  The current
 * number of existing server processes is recorded in rpcNumServers.  Up to
 * rpcMaxServer processes may be created by Rpc_Deamon.  rpcMaxServers may
 * not be set above rpcAbsoluteMaxServers.  (This is for allowing the
 * maximum number of servers to be changed by a system call and thus set
 * in boot scripts differently for different servers.)
 */
extern RpcServerState **rpcServerPtrPtr;
extern int		rpcMaxServers;
extern int		rpcNumServers;
extern int		rpcAbsoluteMaxServers;

/*
 * Whether or not the server should send negative acknowledgements.
 */
extern	Boolean		rpcSendNegAcks;

/*
 * The service procedure switch. This is indexed by procedure number.
 */
typedef ReturnStatus (*IntProc) _ARGS_((ClientData srvToken, int clientID, 
			int command, Rpc_Storage *storagePtr));
typedef struct RpcService {
    IntProc	serviceProc;
    char	*name;
} RpcService;
extern RpcService rpcService[];

/*
 * A histogram of the service times for the different RPCs.
 */
extern Rpc_Histogram	*rpcServiceTime[];

/*
 * A raw count of the number of service calls.
 */
extern int		rpcServiceCount[];

/*
 * For determining whether rpc being handled by this server was sent
 * to the broadcast server ID.
 */
#define	RPC_IS_BROADCAST(srvrPtr)	\
    (((RpcServerState *) srvrPtr)->requestRpcHdr.serverID == RPC_BROADCAST_SERVER_ID)

/*
 * For tracing calls.
 */
extern void Rpc_OkayToTrace _ARGS_((Boolean okay));
extern void Rpc_FreeTraces _ARGS_((void));
extern void RpcAddServerTrace _ARGS_((RpcServerState *srvPtr, RpcHdr *rpcHdrPtr, Boolean noneThere, int num));
extern ReturnStatus Rpc_DumpServerTraces _ARGS_((int length, RpcServerUserStateInfo *resultPtr, int *lengthNeededPtr));

/*
 * Forward declarations.
 */
extern RpcServerState *RpcServerAlloc _ARGS_((RpcHdr *rpcHdrPtr));
extern RpcServerState *RpcServerInstall _ARGS_((void));
extern void RpcServerDispatch _ARGS_((register RpcServerState *srvPtr, register RpcHdr *rpcHdrPtr));
extern RpcServerState *RpcInitServerState _ARGS_((int index));
extern void RpcAck _ARGS_((RpcServerState *srvPtr, int flags));
extern void RpcResend _ARGS_((RpcServerState *srvPtr));
extern void RpcProbe _ARGS_((RpcServerState *srvPtr));
extern void RpcSrvInitHdr _ARGS_((RpcServerState *srvPtr, RpcHdr *rpcHdrPtr, RpcHdr *requestHdrPtr));
extern void RpcSetNackBufs _ARGS_((void));
extern void RpcReclaimServers _ARGS_((Boolean serversMaxed));
extern void RpcInitServerTraces _ARGS_((void));


#endif /* _RPCSERVER */
