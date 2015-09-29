/*
 * rpcDebug.c --
 *
 *	Debugging routines for the Rpc system.  These routines are used
 *	to profile and trace the whole system.
 *
 * Copyright 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/sprited/rpc/RCS/rpcDebug.c,v 1.6 92/07/16 18:06:56 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <mach.h>
#include <status.h>

#include <rpc.h>
#include <rpcInt.h>
#include <fs.h>
#include <fsio.h>
#include <rpcClient.h>
#include <rpcServer.h>
#include <rpcTrace.h>
#include <sig.h>
#include <spriteSrvServer.h>
#include <sync.h>
#include <user/sysStats.h>
#include <timer.h>
#include <vm.h>


Boolean		rpc_SanityCheck = TRUE;


/*
 *----------------------------------------------------------------------
 *
 * Test_RpcStub --
 *
 *	MIG stub for the Rpc testing hook.  
 *
 * Results:
 *	Returns KERN_SUCCESS.  Fills in the Sprite status code and "pending 
 *	signals" flag.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
Test_RpcStub(serverPort, command, argPtr, statusPtr, sigPendingPtr)
    mach_port_t serverPort;	/* request port */
    int command;
    vm_address_t argPtr;
    ReturnStatus *statusPtr;	/* OUT: status code */
    boolean_t *sigPendingPtr;	/* OUT: is there a signal pending */
{
#ifdef lint
    serverPort = serverPort;
#endif
    *statusPtr = Test_Rpc(command, (Address)argPtr);
    *sigPendingPtr = Sig_Pending(Proc_GetCurrentProc());
    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Test_Rpc --
 *
 *	The Rpc testing hook.  This is used to do ECHO and SEND rpcs.
 *
 * Results:
 *	Returns the Sprite status code.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Test_Rpc(command, argPtr)
    int command;
    Address argPtr;
{
    Address inPtr;		/* input buffer */
    int inSize;			/* and its size */
    Address outPtr;		/* output buffer */
    int outSize;		/* and its size */
    int minSize;		/* smaller of inSize and outSize */
    int argSize;
    ReturnStatus status = SUCCESS;
    
    switch(command) {
	case TEST_RPC_ECHO: {
	    Rpc_EchoArgs *echoArgsPtr = (Rpc_EchoArgs *)argPtr;
	    Time deltaTime;

	    /*
	     * Make Accessible the struct containing the Echo arguments,
	     * then the in and out buffers.
	     */
	    Vm_MakeAccessible(VM_READONLY_ACCESS, sizeof(Rpc_EchoArgs),
			   (Address) echoArgsPtr, &argSize,
			   (Address *) (&echoArgsPtr));
	    if (echoArgsPtr == (Rpc_EchoArgs *)NIL) {
		return(RPC_INVALID_ARG);
	    }
	    Vm_MakeAccessible(VM_READONLY_ACCESS,
			      echoArgsPtr->size, echoArgsPtr->inDataPtr,
			      &inSize, &inPtr);
	    if (inPtr == (Address)NIL) {
		Vm_MakeUnaccessible((Address) echoArgsPtr, argSize);
		return(RPC_INVALID_ARG);
	    }
	    /* 
	     * If you make the output region write-only, you'll crash on a 
	     * ds5000 using Mach.  So make it read-write.
	     */
	    Vm_MakeAccessible(VM_READWRITE_ACCESS,
			      echoArgsPtr->size, echoArgsPtr->outDataPtr,
			      &outSize, &outPtr);
	    if (outPtr == (Address)NIL) {
		Vm_MakeUnaccessible(inPtr, inSize);
		Vm_MakeUnaccessible((Address) echoArgsPtr, argSize);
		return(RPC_INVALID_ARG);
	    }
	    minSize = (inSize > outSize) ? outSize : inSize ;

	    status = Rpc_EchoTest(echoArgsPtr->serverID, echoArgsPtr->n,
				  minSize, inPtr, outPtr, &deltaTime);

	    (void) Vm_CopyOut(sizeof(Time), (Address)&deltaTime,
				     (Address)echoArgsPtr->deltaTimePtr);
	    Vm_MakeUnaccessible(inPtr, inSize);
	    Vm_MakeUnaccessible(outPtr, outSize);
	    Vm_MakeUnaccessible((Address) echoArgsPtr, argSize);
	    break;
	}
	case TEST_RPC_SEND: {
	    Rpc_EchoArgs *echoArgsPtr = (Rpc_EchoArgs *)argPtr;
	    Time deltaTime;

	    Vm_MakeAccessible(VM_READONLY_ACCESS, sizeof(Rpc_EchoArgs),
			   (Address) echoArgsPtr, &argSize,
			   (Address *) (&echoArgsPtr));
	    if (argSize != sizeof(Rpc_EchoArgs)) {
		return(RPC_INVALID_ARG);
	    }
	    Vm_MakeAccessible(VM_READONLY_ACCESS,
			      echoArgsPtr->size, echoArgsPtr->inDataPtr,
			      &inSize, &inPtr);
	    if (inPtr == (Address)NIL) {
		Vm_MakeUnaccessible((Address) echoArgsPtr, argSize);
		return(RPC_INVALID_ARG);
	    }

	    status = Rpc_SendTest(echoArgsPtr->serverID, echoArgsPtr->n,
				  inSize, inPtr, &deltaTime);

	    (void) Vm_CopyOut(sizeof(Time), (Address)&deltaTime,
				     (Address)echoArgsPtr->deltaTimePtr);
	    Vm_MakeUnaccessible(inPtr, inSize);
	    Vm_MakeUnaccessible((Address) echoArgsPtr, argSize);
	    break;
	}
	default:
	    status = RPC_INVALID_ARG;
	    break;
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Rpc_GetStats --
 *
 *	Copy out rpc system stats.
 *
 * Results:
 *	SUCCESS, unless an argument could not be made accessible.
 *
 * Side effects:
 *	Fill in the requested statistics.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Rpc_GetStats(command, option, argPtr)
    int command;		/* Specifies what to do */
    int option;			/* Modifier for command */
    Address argPtr;		/* Argument for command */
{
    ReturnStatus status = SUCCESS;
    
    switch(command) {
	case SYS_RPC_ENABLE_SERVICE: {
	    /*
	     * A basic On/Off switch for the RPC system.  Servers in
	     * particular want to get everything ready before responding
	     * to clients.
	     */
	    if (option) {
		if (rpcSendNegAcks) {
		    RpcSetNackBufs();
		}
		printf("Starting RPC service\n");
		rpcServiceEnabled = TRUE;
	    } else {
		printf("Warning: Disabling RPC service\n");
		rpcServiceEnabled = FALSE;
	    }
	    break;
	}
	case SYS_RPC_NUM_NACK_BUFS: {
	    /*
	     * Set the number of negative ack buffers.
	     */
	    if (rpcServiceEnabled) {
		printf(
		    "Rpc service already enabled, cannot change nack bufs.\n");
		break;
	    }
	    if (option > 0) {
		printf("Setting number of nack bufs to %d\n", option);
		rpc_NumNackBuffers = option;
		RpcSetNackBufs();
	    }
	    break;
	}
	case SYS_RPC_SET_MAX: {
	    /*
	     * Set the maximum number of server procs allowed.  This can't
	     * go higher than the absolute maximum.
	     */
	    if (option <= 0) {
		printf("Warning: asked to set max number of rpc server ");
		printf("procs to %d.  I won't do this.\n", option);
	    } if (option < rpcNumServers) {
		printf("Warning: asked to set max number of rpc server ");
		printf("procs to %d which is less than the current ", option);
		printf("number of procs: %d.\n", rpcNumServers);
		printf("I can't do this.\n");
	    } else if (option > rpcAbsoluteMaxServers) {
		printf("Warning: asked to set max number of rpc server ");
		printf("procs to %d which is above the absolute ", option);
		printf("maximum of %d.\n", rpcAbsoluteMaxServers);
		printf("Setting this to maximum instead.\n");
		rpcMaxServers = rpcAbsoluteMaxServers;
	    } else {
		printf("Setting max number of server procs to %d.\n", option);
		rpcMaxServers = option;
	    }
	    break;
	}
	case SYS_RPC_SET_NUM: {
	    /*
	     * Set the number of rpc processes to the given number, if
	     * there aren't already that many.
	     */
	    int	pid;

	    if (option <= 0) {
		printf("Warning: asked to set number of rpc server ");
		printf("procs to %d.  I won't do this.\n", option);
	    } else if (option < rpcNumServers) {
		printf("There are already %d server procs, ", rpcNumServers);
		printf("and I won't kill them.\n");
	    } else if (option > rpcMaxServers) {
		printf("Warning: asked to set number of rpc server ");
		printf("procs to %d, which is above the maximum of %d\n",
			option, rpcMaxServers);
		printf("I'll create up to the maximum.\n");
		while (rpcNumServers < rpcMaxServers) {
		    if (Rpc_CreateServer(&pid) == SUCCESS) {
			printf("RPC srvr %x\n", pid);
			RpcResetNoServers(0);
		    } else {
			printf("Warning: no more RPC servers\n");
			RpcResetNoServers(-1);
		    }
		}
	    } else {
		/* create the correct number */
		while (rpcNumServers < option) {
		    if (Rpc_CreateServer(&pid) == SUCCESS) {
			printf("RPC srvr %x\n", pid);
			RpcResetNoServers(0);
		    } else {
			printf("Warning: no more RPC servers\n");
			RpcResetNoServers(-1);
		    }
		}
	    }
	    break;
	}
	case SYS_RPC_NEG_ACKS: {
	    printf("Turning negative acknowledgements %s.\n",
		    option ? "on" : "off");
	    rpcSendNegAcks = option;
	    break;
	}
	case SYS_RPC_CHANNEL_NEG_ACKS: {
	    printf("Turning client policy of ramping down channels %s.\n",
		    option ? "on" : "off");
	    rpcChannelNegAcks = option;
	    break;
	}
	case SYS_RPC_CLT_STATS: {
	    register Rpc_CltStat *cltStatPtr;

	    cltStatPtr = (Rpc_CltStat *)argPtr;
	    if (cltStatPtr == (Rpc_CltStat *)NIL ||
		cltStatPtr == (Rpc_CltStat *)0 ||
		cltStatPtr == (Rpc_CltStat *)USER_NIL) {
		
		Rpc_PrintCltStat();
	    } else {
		RpcResetCltStat();
		status = Vm_CopyOut(sizeof(Rpc_CltStat),
				  (Address)&rpcTotalCltStat,
				  (Address) cltStatPtr);
	    }
	    break;
	}
	case SYS_RPC_SRV_STATS: {
	    register Rpc_SrvStat *srvStatPtr;

	    srvStatPtr = (Rpc_SrvStat *)argPtr;
	    if (srvStatPtr == (Rpc_SrvStat *)NIL ||
		srvStatPtr == (Rpc_SrvStat *)0 ||
		srvStatPtr == (Rpc_SrvStat *)USER_NIL) {
		
		Rpc_PrintSrvStat();
	    } else {
		RpcResetSrvStat();
		status = Vm_CopyOut(sizeof(Rpc_SrvStat),
				  (Address)&rpcTotalSrvStat,
				  (Address) srvStatPtr);
	    }
	    break;
	}
	case SYS_RPC_TRACE_STATS: {
	    switch(option) {
		case SYS_RPC_TRACING_PRINT:
		    Rpc_PrintTrace((ClientData)32);
		    break;
		case SYS_RPC_TRACING_ON:
		    rpc_Tracing = TRUE;
		    break;
		case SYS_RPC_TRACING_OFF:
		    rpc_Tracing = FALSE;
		    break;
		default:
		    /*
		     * The option is the size of the users buffer to
		     * hold all the trace records.
		     */
		    status = Trace_Dump(rpcTraceHdrPtr, RPC_TRACE_LEN, argPtr);
		    break;
	    }
	    break;
	}
	case SYS_RPC_SERVER_HIST: {
	    /*
	     * Operate on the service-time histograms, depending on option.
	     */
	    if (option > 0 && option <= RPC_LAST_COMMAND) {
		/*
		 * Copy out the histogram for the service time of the RPC
		 * indicated by option.
		 */
		status = Rpc_HistDump(rpcServiceTime[option], argPtr);
	    } else if (option > RPC_LAST_COMMAND) {
		status = RPC_INVALID_ARG;
	    } else {
		/*
		 * Reset all the server side histograms
		 */
		status = SUCCESS;
		for (option = 1 ; option <= RPC_LAST_COMMAND ; option++) {
		    Rpc_HistReset(rpcServiceTime[option]);
		}
	    }
	    break;
	}
	case SYS_RPC_CLIENT_HIST: {
	    /*
	     * Operate on the client call-time histograms, depending on option.
	     */
	    if (option > 0 && option <= RPC_LAST_COMMAND) {
		/*
		 * Copy out the histogram for the client's view of the service
		 * time of the RPC indicated by option.
		 */
		status = Rpc_HistDump(rpcCallTime[option], argPtr);
	    } else if (option > RPC_LAST_COMMAND) {
		status = RPC_INVALID_ARG;
	    } else {
		/*
		 * Reset all the client side histograms
		 */
		status = SUCCESS;
		for (option = 1 ; option <= RPC_LAST_COMMAND ; option++) {
		    Rpc_HistReset(rpcCallTime[option]);
		}
	    }
	    break;
	}
	case SYS_RPC_SRV_STATE: {
	    /*
	     * Return the state of the server processes.
	     */
	    if (option >= 0 && option < rpcNumServers) {
		/*
		 * Copy out the state of the option'th server process.
		 */
		status = Vm_CopyOut(sizeof(RpcServerState),
				  (Address)rpcServerPtrPtr[option], argPtr);
	    } else {
		status = RPC_INVALID_ARG;
	    }
	    break;
	}
	case SYS_RPC_CLT_STATE: {
	    /*
	     * Return the state of the client channels.
	     */
	    if (option >= 0 && option < rpcNumChannels) {
		/*
		 * Copy out the state of the option'th client channel.
		 */
		status = Vm_CopyOut(sizeof(RpcClientChannel),
				  (Address)rpcChannelPtrPtr[option], argPtr);
	    } else {
		status = RPC_INVALID_ARG;
	    }
	    break;
	}
	case SYS_RPC_CALL_COUNTS: {
	    register int *callCountPtr;

	    callCountPtr = (int *)argPtr;
	    if (callCountPtr == (int *)NIL ||
		callCountPtr == (int *)0 ||
		callCountPtr == (int *)USER_NIL ||
		option <= 0) {
		
		Rpc_PrintCallCount();
	    } else {
		status = Vm_CopyOut(option,
				  (Address) rpcClientCalls,
				  (Address) callCountPtr);
	    }
	    break;
	}
	case SYS_RPC_SRV_COUNTS: {
	    register int *serviceCountPtr;

	    serviceCountPtr = (int *)argPtr;
	    if (serviceCountPtr == (int *)NIL ||
		serviceCountPtr == (int *)0 ||
		serviceCountPtr == (int *)USER_NIL ||
		option <= 0) {
		
		Rpc_PrintServiceCount();
	    } else {
		status = Vm_CopyOut(option,
				  (Address) rpcServiceCount,
				  (Address) serviceCountPtr);
	    }
	    break;
	}
	case SYS_RPC_SANITY_CHECK: {
	    printf("Turning RPC sanity checking %s, was %s.\n",
		    option ? "on" : "off",
		    rpc_SanityCheck ? "on" : "off");
	    rpc_SanityCheck = option;
	    break;
	}
	default:
	    status = RPC_INVALID_ARG;
	    break;
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Rpc_SanityCheck --
 *
 *	Do some sanity checks on the packet.  This is not meant to be
 *	an exhaustive test of the validity of a packet; rather it
 *	is intended to be a debugging aid for weeding out bad packets
 *	from clients.
 *
 * Results:
 *	SUCCESS if the packet looks ok, FAILURE otherwise
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Rpc_SanityCheck(length, scatterPtr, packetLength)
    int			length;		/* Length of the scatter/gather 
					 * array. */	
    Net_ScatterGather	*scatterPtr; 	/* The scatter/gather array. */
    int			packetLength;	/* Length of the network packet. */

{
    static int		buffer[(RPC_MAX_SIZE + sizeof(RpcHdr)) / sizeof(int)];
    RpcHdr		*rpcHdrPtr = NULL;
    int			rpcLength;
    ReturnStatus	status = SUCCESS;
    char		*packetPtr;
    int			paramSize;

    if (packetLength < sizeof(RpcHdr)) {
	printf("RpcSanityCheck: packet smaller than an RpcHdr (%d)\n",
	    packetLength);
	return FAILURE;
    }
    if (length == 1) {
	rpcHdrPtr = (RpcHdr *) scatterPtr->bufAddr;
	packetPtr = (char *) scatterPtr->bufAddr;
    } else {
	Net_GatherCopy(scatterPtr, length, 
	    (Address) buffer);
	rpcHdrPtr = (RpcHdr *) buffer;
	packetPtr = (char *) buffer;
    }
    if ((int) rpcHdrPtr & 0x3) {
	printf("rpcHdrPtr = 0x%x\n", rpcHdrPtr);
	rpc_SanityCheck = FALSE;
	panic("Bye\n");
    }
    rpcLength = sizeof(RpcHdr) + rpcHdrPtr->paramSize + rpcHdrPtr->dataSize;
    if (rpcLength > packetLength) {
	printf("RpcSanityCheck: packet too short, %d < %d\n", 
	    packetLength, rpcLength);
	status = FAILURE;
	goto done;
    }
    /*
     * We only deal with the first fragment of an RPC.
     */

    if ((rpcHdrPtr->numFrags != 0) && (rpcHdrPtr->fragMask != 0x1)) {
	return SUCCESS;
    }
    paramSize = rpcHdrPtr->paramSize;
    if (rpcHdrPtr->flags & RPC_REQUEST) {
	switch(rpcHdrPtr->command) {
	    case RPC_FS_OPEN:
	    case RPC_FS_READ:
	    case RPC_FS_WRITE:
	    case RPC_FS_CLOSE:
	    case RPC_FS_UNLINK:
	    case RPC_FS_MKDIR:
	    case RPC_FS_RMDIR:
	    case RPC_FS_MKDEV:
	    case RPC_FS_GET_ATTR:
	    case RPC_FS_SET_ATTR:
	    case RPC_FS_GET_ATTR_PATH:
	    case RPC_FS_SET_ATTR_PATH:
	    case RPC_FS_SET_IO_ATTR:
	    case RPC_FS_DEV_OPEN:
	    case RPC_FS_SELECT: 
	    case RPC_FS_IO_CONTROL:
	    case RPC_FS_CONSIST:
	    case RPC_FS_CONSIST_REPLY:
	    case RPC_FS_COPY_BLOCK:
	    case RPC_FS_MIGRATE:
	    case RPC_FS_RELEASE:
	    case RPC_FS_REOPEN:
	    case RPC_FS_DOMAIN_INFO: {
		Fs_FileID	*fileIDPtr;
		if (paramSize < sizeof(Fs_FileID)) {
		    printf("Rpc_SanityCheck: request %d param too small (%d)\n",
			paramSize);
		    status = FAILURE;
		    break;
		}
		fileIDPtr = (Fs_FileID *) (packetPtr + sizeof(RpcHdr));
		if ((int) packetPtr & 0x3) {
		    printf("packetPtr = 0x%x\n", packetPtr);
		    panic("Bye\n");
		}
		if ((fileIDPtr->type < 0) || 
		    (fileIDPtr->type >= FSIO_NUM_STREAM_TYPES)) {
		    printf("Rpc_SanityCheck: request %d file id type = %d\n", 
			rpcHdrPtr->command, fileIDPtr->type);
		    status = FAILURE;
		}
		break;
	    }
	}
    }

done:
    if (status != SUCCESS) {
	printf("Rpc_SanityCheck: client %d, server %d:\n", 
	    rpcHdrPtr->clientID, rpcHdrPtr->serverID);
    }
    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * Rpc_NotImplemented --
 *
 *	Temporary replacement for RPC stubs that aren't yet installed.
 *
 * Results:
 *	Returns FAILURE, so that the caller will send an error reply.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ReturnStatus
Rpc_NotImplemented(srvToken, clientID, command, storagePtr)
    ClientData srvToken;
    int clientID;		/* Sprite ID of client host */
    int command;		/* which RPC request */
    Rpc_Storage *storagePtr;
{
    char hostName[128];
    char rpcName[RPC_MAX_NAME_LENGTH];
    
    Net_SpriteIDToName(clientID, sizeof(hostName), hostName);
    Rpc_GetName(command, sizeof(rpcName), rpcName);
    printf("Can't process ``%s'' RPC (%d) for client %s (%d): unimplemented\n",
	   rpcName, command, hostName, clientID);

    return FAILURE;
}
