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
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include "sprite.h"
#include "status.h"
#include "rpc.h"
#include "fs.h"
#include "timer.h"
#include "vm.h"
#include "sync.h"
#include "sched.h"
#include "rpcClient.h"
#include "rpcServer.h"
#include "rpcTrace.h"
#include "sched.h"
#include "dev.h"
#include "user/test.h"
#include "user/sysStats.h"



/*
 *----------------------------------------------------------------------
 *
 * Test_RpcStub --
 *
 *	System call stub for the Rpc testing hook.  This is used
 *	to do ECHO and SEND rpcs.
 *
 * Results:
 *	An error code from the RPC.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Test_RpcStub(command, argPtr)
    int command;
    Address argPtr;
{
    int inSize;
    int outSize;
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
			      &inSize, &echoArgsPtr->inDataPtr);
	    if (echoArgsPtr->inDataPtr == (Address)NIL) {
		Vm_MakeUnaccessible((Address) echoArgsPtr, argSize);
		return(RPC_INVALID_ARG);
	    }
	    Vm_MakeAccessible(VM_OVERWRITE_ACCESS,
			      echoArgsPtr->size, echoArgsPtr->outDataPtr,
			      &outSize, &echoArgsPtr->outDataPtr);
	    if (echoArgsPtr->outDataPtr == (Address)NIL) {
		Vm_MakeUnaccessible(echoArgsPtr->inDataPtr, inSize);
		Vm_MakeUnaccessible((Address) echoArgsPtr, argSize);
		return(RPC_INVALID_ARG);
	    }
	    echoArgsPtr->size = (inSize > outSize) ? outSize : inSize ;

	    status = Rpc_EchoTest(echoArgsPtr->serverID, echoArgsPtr->n,
				echoArgsPtr->size, echoArgsPtr->inDataPtr,
				echoArgsPtr->outDataPtr, &deltaTime);

	    (void) Vm_CopyOut(sizeof(Time), (Address)&deltaTime,
				     (Address)echoArgsPtr->deltaTimePtr);
	    Vm_MakeUnaccessible(echoArgsPtr->inDataPtr, inSize);
	    Vm_MakeUnaccessible(echoArgsPtr->outDataPtr, outSize);
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
			      &inSize, &echoArgsPtr->inDataPtr);
	    if (echoArgsPtr->inDataPtr == (Address)NIL) {
		Vm_MakeUnaccessible((Address) echoArgsPtr, argSize);
		return(RPC_INVALID_ARG);
	    }

	    status = Rpc_SendTest(echoArgsPtr->serverID, echoArgsPtr->n,
				inSize, echoArgsPtr->inDataPtr, &deltaTime);

	    (void) Vm_CopyOut(sizeof(Time), (Address)&deltaTime,
				     (Address)echoArgsPtr->deltaTimePtr);
	    Vm_MakeUnaccessible(echoArgsPtr->inDataPtr, inSize);
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
		Sys_Printf("Starting RPC service\n");
		rpcServiceEnabled = TRUE;
	    } else {
		Sys_Panic(SYS_WARNING, "Disabling RPC service\n");
		rpcServiceEnabled = FALSE;
	    }
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
		    Rpc_PrintTrace(32);
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
		status == SUCCESS;
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
		status == SUCCESS;
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
	default:
	    status = RPC_INVALID_ARG;
	    break;
    }
    return(status);
}
