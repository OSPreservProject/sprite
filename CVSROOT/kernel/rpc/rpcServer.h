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

#include "net.h"
#include "sync.h"
#include "timer.h"

#include "rpcInt.h"
#include "rpcSrvStat.h"
#include "rpcHistogram.h"



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
    int			mutex;
    Sync_Condition	waitCondition;

    /*
     * This bitmask indicates which fragments our client has recieved.
     * It is used by RpcOutput for partial resends.
     */
    int			fragsDelivered;
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
    int			(*freeReplyProc)();
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
 */
#define SRV_NOTREADY	0x00
#define SRV_FREE	0x01
#define SRV_BUSY	0x02
#define SRV_WAITING	0x04
#define SRV_AGING	0x08
#define SRV_FRAGMENT	0x10
#define SRV_NO_REPLY	0x20

/*
 * The server's state table has a maximum number of entries, but not all
 * the entries are initialized and have an associated process.  The current
 * number of existing server processes is recorded in rpcNumServers.  Up to
 * rpcMaxServer processes may be created by Rpc_Deamon.
 */
extern RpcServerState **rpcServerPtrPtr;
extern int		rpcMaxServers;
extern int		rpcNumServers;

/*
 * The service procedure switch. This is indexed by procedure number.
 */
typedef int		(*IntProc)();
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
 * Forward declarations.
 */
RpcServerState		*RpcServerAlloc();
RpcServerState		*RpcServerInstall();
RpcServerState		*RpcInitServerState();
void			 RpcServerDispatch();
void			 RpcAck();
void			 RpcResend();
void			 RpcProbe();
void			 RpcSrvInitHdr();

#endif _RPCSERVER
