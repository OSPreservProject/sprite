/*
 * rpcInit.c --
 *
 *	Initialize the data structures needed by the RPC system.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include "sprite.h"
#include "rpc.h"
#include "rpcClient.h"
#include "rpcServer.h"
#include "rpcTrace.h"
#include "vm.h"
#include "timer.h"
#include "net.h"

/*
 * These are the rpc version numbers, both in native format and in
 * byte-swapped format. 
 */

int	rpc_NativeVersion = RPC_NATIVE_VERSION;
int	rpc_SwappedVersion = RPC_SWAPPED_VERSION;

/*
 * Constant parameters for the protocol.  The RpcConst structure keeps
 * the complete set of constants.  Two sets of constants are defined,
 * one that is appropriate for a local ethernet, one that has longer
 * timeouts and is better for talking across gateways.
 */
RpcConst rpcEtherConst;
RpcConst rpcInetConst;

void RpcBufferInit();


/*
 *----------------------------------------------------------------------
 *
 * Rpc_Init --
 *
 *      Allocate and set up the tables used by the RPC system.  This
 *      should be called after virtual memory allocation can be done and
 *      before any RPCs are attempted.  This allocates the Client Channel
 *	data structures and some stuff for the Rpc Servers' state.  The
 *	number of client channels is fixed by rpcNumChannels, but the
 *	number of RPC server processes can grow via the Rpc_Deamon process.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Allocate space for tables, and set the initial state for the
 *	client channels and the servers.
 *
 *----------------------------------------------------------------------
 */
void
Rpc_Init()
{
    register int i;
    register int frag;
    Net_EtherAddress etherAddress;
    int	     maxHdrSize;

    /*
     * Initialize sets of time parameters.  These structures are used in
     * the RpcDoCall code.  This is much too hard coded, and someone
     * should figure out how to dynamically determine these parameters.
     *
     * Ethernet - retry after 0.1 seconds, .5 seconds if fragmenting.
     *	Double this until the retry interval is 1 second.
     *	Retry at most 8 times, for a total timeout period of
     *		.1 + .2 + .4 + .8 + 1.0 + 1.0 + 1.0 + 1.0 = 5.5
     *		.5 + 1.0 + 1.0 + 1.0 + 1.0 + 1.0 + 1.0 + 1.0 = 7.5 (frag)
     *	If we are recieving acks, then back off and probe every 5 seconds.
     *	If we get acked 10 times then give a warning.
     */
    rpcEtherConst.retryMsec = 100;
    rpcEtherConst.retryWait = 100 * timer_IntOneMillisecond;
    rpcEtherConst.fragRetryMsec = 500;
    rpcEtherConst.fragRetryWait = 500 * timer_IntOneMillisecond;
    rpcEtherConst.maxAckMsec = 5000;
    rpcEtherConst.maxAckWait = 5000 * timer_IntOneMillisecond;
    rpcEtherConst.maxTimeoutMsec = 1000;
    rpcEtherConst.maxTimeoutWait = 1000 * timer_IntOneMillisecond;
    rpcEtherConst.maxTries = 8;
    rpcEtherConst.maxAcks = 10;
    /*
     * Internet (IP) - retry after 0.5 seconds, 1.0 if fragmenting.
     *	Double this until the retry interval is 4 seconds.
     *	Retry at most 8 times, for a total timeout period of
     *		.5 + 1.0 + 2.0 + 4.0 + 4.0 + 4.0 + 4.0 + 4.0 = 23.5
     *		1.0 + 2.0 + 4.0 + 4.0 + 4.0 + 4.0 + 4.0 + 4.0 = 27.0 (frag)
     *	If we are recieving acks, then back off and probe every 5 seconds.
     *	If we get acked 10 times then give a warning.
     */
    rpcInetConst.retryMsec = 500;
    rpcInetConst.retryWait = 500 * timer_IntOneMillisecond;
    rpcInetConst.fragRetryMsec = 1000;
    rpcInetConst.fragRetryWait = 1000 * timer_IntOneMillisecond;
    rpcInetConst.maxAckMsec = 5000;
    rpcInetConst.maxAckWait = 5000 * timer_IntOneMillisecond;
    rpcInetConst.maxTimeoutMsec = 4000;
    rpcInetConst.maxTimeoutWait = 4000 * timer_IntOneMillisecond;
    rpcInetConst.maxTries = 8;
    rpcInetConst.maxAcks = 10;

    /*
     * TRACE and HISTOGRAM initialization.
     */
    Trace_Init(rpcTraceHdrPtr, RPC_TRACE_LEN, sizeof(RpcHdr), 0);

    rpcServiceTime[0] = (Rpc_Histogram *)NIL;
    rpcCallTime[0] = (Rpc_Histogram *)NIL;
    for (i=1 ; i<=RPC_LAST_COMMAND ; i++) {
	rpcServiceTime[i] = Rpc_HistInit(RPC_NUM_HIST_BUCKETS, 1024);
	rpcCallTime[i] = Rpc_HistInit(RPC_NUM_HIST_BUCKETS, 1024);
    }

    /*
     * Set our preferred inter-fragment delay based on machine type.
     * This is a microsecond value.  Our output rate starts the same
     * as the input rate, although MyDelay could increase if a machine
     * senses that it is overloaded.
     */

    RpcGetMachineDelay(&rpcMyDelay, &rpcOutputRate);

    /*
     * The client channel table is kept as a pointer to an array of pointers
     * to client channels.  First allocate the table of pointers and then
     * allocate storage for each channel.
     */
    rpcChannelPtrPtr = (RpcClientChannel **)
	    Vm_RawAlloc(rpcNumChannels * sizeof(RpcClientChannel *));

    /*
     * Query the net module of the maximum size protocol header buffer 
     * needed.
     */
    for (i=0 ; i<rpcNumChannels ; i++) {
	register RpcClientChannel *chanPtr;

	chanPtr = (RpcClientChannel *)Vm_RawAlloc(sizeof(RpcClientChannel));
	rpcChannelPtrPtr[i] = chanPtr;

	chanPtr->state = CHAN_FREE;
	chanPtr->index = i;
	chanPtr->serverID = -1;
	Sync_SemInitDynamic(&chanPtr->mutex,"Rpc:RpcClientChannel.mutex");
	Sync_SemRegister(&chanPtr->mutex);
	chanPtr->waitCondition.waiting = FALSE;

	/*
	 * Set up header storage and the scatter/gather sets used to
	 * refer to a whole message.  This is done for each type
	 * of packet (request, reply ack), plus an array of these
	 * things used for fragmenting our request.
	 */
	RpcBufferInit(&chanPtr->requestRpcHdr, &chanPtr->request,
			chanPtr->index, -1);
	RpcBufferInit(&chanPtr->replyRpcHdr, &chanPtr->reply,
			chanPtr->index, -1);
	RpcBufferInit(&chanPtr->ackHdr, &chanPtr->ack,
			chanPtr->index, -1);

	for (frag=0 ; frag < RPC_MAX_NUM_FRAGS ; frag++) {
	    RpcBufferInit(&chanPtr->fragRpcHdr[frag], &chanPtr->fragment[frag],
			    chanPtr->index, -1);
	}

    }

    /*
     * Initialize the servers' state table.  Most slots are left
     * uninitialized.  They get filled in by Rpc_Deamon when it creates
     * new server processes.  After creation, a server process
     * claims a table entry with RpcServerInstall.
     */
    rpcServerPtrPtr = (RpcServerState **)
	    Vm_RawAlloc(rpcMaxServers * sizeof(RpcServerState *));
    for (i=0 ; i<rpcMaxServers ; i++) {
	rpcServerPtrPtr[i] = (RpcServerState *)NIL;
    }

    /*
     * Ask the net module to set up our Sprite ID.  It uses either
     * existing (compiled in) addresses or Reverse ARP.  If we can't
     * figure out our ID we use zero and rely on the RPC server to
     * propogate our Sprite ID back in the first RPC reply message.
     */
    Mach_GetEtherAddress(&etherAddress);
    rpc_SpriteID = Net_AddrToID(0, NET_ROUTE_ETHER, (ClientData)&etherAddress);
    if (rpc_SpriteID < 0) {
	rpc_SpriteID = 0;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * RpcBufferInit --
 *
 *	Initialize a packet buffer for one of the various packets sent
 *	via an RPC channel.  They all share the same packet format and
 *	buffering system, and this call is used to initialize it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Allocate memory with Vm_RawAlloc.  Give initial values to
 *	the fields of the RPC header that never change.  These are:
 *	version		# byte ordering version.
 *	channel		# channel port number.
 *	delay		# interfragment delay.
 *	paramSize 0	# size of the parameter area of the message.
 *	dataSize 0	# size of the data area of the message.
 *	numFrags 0	# >0 if fragmented
 *	fragMask 0	# fragmentID
 *
 *----------------------------------------------------------------------
 */
void
RpcBufferInit(rpcHdrPtr, bufferSetPtr, channel, serverHint)
    RpcHdr *rpcHdrPtr;		/* Storage for packet header */
    RpcBufferSet *bufferSetPtr;	/* Scatter/gather vector for whole message */
    int channel;		/* chanPtr->index */
    int serverHint;		/* srvPtr->index */
{
    int maxHdrSize = Net_MaxProtoHdrSize();

    bufferSetPtr->protoHdrBuffer.length = maxHdrSize;
    bufferSetPtr->protoHdrBuffer.bufAddr = Vm_RawAlloc(maxHdrSize);
    bufferSetPtr->protoHdrBuffer.mutexPtr = (Sync_Semaphore *)NIL;

    bufferSetPtr->rpcHdrBuffer.length = sizeof(RpcHdr);
    bufferSetPtr->rpcHdrBuffer.bufAddr = (Address)rpcHdrPtr;
    bufferSetPtr->rpcHdrBuffer.mutexPtr = (Sync_Semaphore *)NIL;

    bufferSetPtr->paramBuffer.length = 0;
    bufferSetPtr->paramBuffer.bufAddr = (Address)NIL;
    bufferSetPtr->paramBuffer.mutexPtr = (Sync_Semaphore *)NIL;

    bufferSetPtr->dataBuffer.length = 0;
    bufferSetPtr->dataBuffer.bufAddr = (Address)NIL;
    bufferSetPtr->dataBuffer.mutexPtr = (Sync_Semaphore *)NIL;
    /*
     * Set up RPC header fields that don't change.
     */
    rpcHdrPtr->version = rpc_NativeVersion;
    rpcHdrPtr->delay = rpcMyDelay;

    rpcHdrPtr->clientID = rpc_SpriteID;
    rpcHdrPtr->channel = channel;
    rpcHdrPtr->serverID = rpc_SpriteID;
    rpcHdrPtr->serverHint = serverHint;
    /*
     * And some that might not have to change.
     */
    rpcHdrPtr->numFrags = 0;
    rpcHdrPtr->fragMask = 0;
    rpcHdrPtr->paramSize = 0;
    rpcHdrPtr->dataSize = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * RpcInitServerState --
 *
 *	Initialize a server state table entry.  This is called before
 *	a server process is created to set up its state.  The state is
 *	used as the primary communication mechanism between the server
 *	process and the rest of the world.
 *
 * Results:
 *	A pointer to an initialized server state table entry.  This
 *	value needs to be saved in a table somewhere by the caller.
 *
 * Side effects:
 *	Allocate memory with Vm_RawAlloc.  Give initial values to
 *	all the elements of the table entry.  The "state" field of
 *	the table is set to SRV_NOTREADY and a server process has
 *	to claim the table entry with RpcServerInstall.
 *
 *----------------------------------------------------------------------
 */
RpcServerState *
RpcInitServerState(index)
    int index;		/* Caller's index of returned info.  This is saved
			 * in the table and used as a hint to clients */
{
    register RpcServerState *srvPtr;	/* Server state that is initialized */
    register int frag;			/* Index into array of headers used
					 * for fragmenting */
    static Sync_Semaphore mutexInit =
	Sync_SemInitStatic("RpcServerState->mutex");

    Sync_SemRegister(&mutexInit);

    srvPtr = (RpcServerState *)Vm_RawAlloc(sizeof(RpcServerState));

    srvPtr->state = SRV_NOTREADY;
    srvPtr->ID = 0;
    srvPtr->freeReplyProc = (int (*)())NIL;
    srvPtr->freeReplyData = (ClientData)NIL;
    srvPtr->index = index;
    srvPtr->clientID = -1;
    srvPtr->channel = -1;
    srvPtr->mutex = mutexInit;
    srvPtr->waitCondition.waiting = FALSE;

    /*
     * Set up the buffer address for the RPC header of replies
     * and acks to point to the headers kept here in the server's state.
     */
    RpcBufferInit(&srvPtr->replyRpcHdr, &srvPtr->reply, -1, srvPtr->index);
    RpcBufferInit(&srvPtr->ackRpcHdr, &srvPtr->ack, -1, srvPtr->index);
    for (frag=0 ; frag < RPC_MAX_NUM_FRAGS ; frag++) {
	RpcBufferInit(&srvPtr->fragRpcHdr[frag], &srvPtr->fragment[frag],
			-1, srvPtr->index);
    }
    /*
     * Set up the scatter vector for input requests to the server.
     * Allocate buffer space for the largest possible request.
     */
    RpcBufferInit(&srvPtr->requestRpcHdr, &srvPtr->request, -1, srvPtr->index);
    srvPtr->request.paramBuffer.bufAddr = Vm_RawAlloc(RPC_MAX_PARAM_SIZE);
    srvPtr->request.paramBuffer.length = RPC_MAX_PARAM_SIZE;

    srvPtr->request.dataBuffer.bufAddr = Vm_RawAlloc(RPC_MAX_DATA_SIZE);
    srvPtr->request.dataBuffer.length = RPC_MAX_DATA_SIZE;

    /*
     * Initialize temporaries.
     */
    srvPtr->actualParamSize = 0;
    srvPtr->actualDataSize = 0;

    return(srvPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Rpc_Start --
 *
 *      Conduct the preliminary RPC's neccesary to start up the client
 *      side of the RPC system.  A Get Time RPC is done to initialize the
 *      boot time stamp.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Do a Get Time RPC to initialize rpcBootID;
 *
 *----------------------------------------------------------------------
 */
void
Rpc_Start()
{
    Time bootTime;	/* Time returned from the default server */
    int tzMinutes;	/* Minutes west of Greenwich */
    int tzDST;		/* Daylight savings flag */
    ReturnStatus status;	/* Status code from the RPC */
    char dateString[40];/* To hold a printable version of the time */
    Net_EtherAddress etherAddr;
    int seconds;
    int spriteID;

    /*
     * Do a Sprite reverse Arp to discover our Sprite ID.  If it's still
     * zero after this that inhibits the RPC system.  In that case we'd
     * better be a diskfull machine so we find out our SpriteID by
     * the user program that installs routes.  See Net_InstallRoute.
     */
    
    Mach_GetEtherAddress(&etherAddr);
    spriteID = Net_RevArp(NET_ROUTE_ETHER, &etherAddr);
    if (spriteID > 0) {
	rpc_SpriteID = spriteID;
	printf("Reverse Arp, setting Sprite ID to %d\n", spriteID);
    }

    Rpc_StampTest();

    status = Rpc_GetTime(RPC_BROADCAST_SERVER_ID, &bootTime, &tzMinutes,
						 &tzDST);
    if (status != SUCCESS) {
	Timer_Ticks ticks;

	printf("Rpc_Start: error (%x) from Get Time RPC\n", status);
	Timer_GetCurrentTicks(&ticks);
	Timer_TicksToTime(ticks, &bootTime);
    } else {
	Timer_SetTimeOfDay(bootTime, tzMinutes, tzDST);
    }
    rpcBootID = bootTime.seconds;

    /*
     * Convert from Greenwich Standard minutes to local minutes
     * and print the time on the console.
     */
    seconds = bootTime.seconds + tzMinutes * 60;
    Time_ToAscii(seconds, FALSE, dateString);
    printf("%s\n", dateString);
}

/*
 *----------------------------------------------------------------------
 *
 * Rpc_MaxSizes --
 *
 *      This function returns the maximum amount of data that can be sent
 *      in one RPC.  A remote procedure has its inputs and outputs packed
 *      into two buffers called the "data area" and the "parameter area".
 *      Two values are returned, the maximums for the parameter and data
 *      areas.
 *
 * Results:
 *	The first parameter gets the maximum data size, the
 *	second gets the maximum parameter size.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
Rpc_MaxSizes(maxDataSizePtr, maxParamSizePtr)
    int *maxDataSizePtr;
    int *maxParamSizePtr;
{
    if (maxDataSizePtr != (int *)NIL){
	*maxDataSizePtr = RPC_MAX_DATASIZE;
    }
    if (maxParamSizePtr != (int *)NIL){
	*maxParamSizePtr = RPC_MAX_PARAMSIZE;
    }
}
