/* 
 * rpcTest.c --
 *
 *	These are some utility routines that exercise the RPC system.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "rpc.h"
#include "rpcTrace.h"
#include "rpcTrace.h"
#include "rpcSrvStat.h"
#include "rpcCltStat.h"
#include "time.h"
#include "timer.h"
#include "proc.h"
#include "mem.h"
#include "byte.h"


/*
 *----------------------------------------------------------------------
 *
 * Rpc_GetTime --
 *
 *	Get the time of day (in seconds since 1970) from the
 *	specified server.
 *
 * Results:
 *	The status code from the RPC. 0 means all went well.
 *
 * Side effects:
 *	Fill in the time argument with the value returned from
 *	the server.  The time is cleared to zeros if there is
 *	an status from the server.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Rpc_GetTime(serverId, timePtr, timeZoneMinutesPtr, timeZoneDSTPtr)
    int serverId;
    Time *timePtr;
    int *timeZoneMinutesPtr;
    int *timeZoneDSTPtr;
{
    Rpc_Storage storage;
    ReturnStatus status;
    struct RpcTimeReturn {
	Time	time;
	int	timeZoneMinutes;
	int	timeZoneDST;
    } rpcTimeReturn;

    storage.requestDataPtr = (Address)NIL;
    storage.requestDataSize = 0;

    storage.requestParamPtr = (Address)NIL;
    storage.requestParamSize = 0;

    storage.replyDataPtr = (Address)NIL;
    storage.replyDataSize = 0;

    storage.replyParamPtr = (Address)&rpcTimeReturn;
    storage.replyParamSize = sizeof(rpcTimeReturn);

    status = Rpc_Call(serverId, RPC_GETTIME, &storage);
    if (status) {
	timePtr->seconds = 0;
	timePtr->microseconds = 0;
	*timeZoneMinutesPtr = 0;
	*timeZoneDSTPtr = 0;
    } else {
	*timePtr = rpcTimeReturn.time;
	*timeZoneMinutesPtr = rpcTimeReturn.timeZoneMinutes;
	*timeZoneDSTPtr = rpcTimeReturn.timeZoneDST;
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Rpc_EchoTest --
 *
 *	Conduct a series of Echoes off the specified server.
 *
 * Results:
 *	The status code from the RPC.
 *
 * Side effects:
 *	Those of Rpc_Echo
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Rpc_EchoTest(serverId, numEchoes, size, inputPtr, returnPtr, deltaTimePtr)
    int serverId;
    int numEchoes;
    int size;
    Address inputPtr;
    Address returnPtr;
    Time *deltaTimePtr;		/* Return: the average time per RPC.  If
				 * this is a NIL pointer then the results
				 * are printed to the console */
{
    int			packetId;
    ReturnStatus	status;
    Timer_Ticks		startTime;
    Timer_Ticks		endTime;
    Time		diff;
    Address		localInBuffer;
    Address		localOutBuffer;

    localInBuffer = Mem_Alloc(size);
    Byte_Copy(size, inputPtr, localInBuffer);
    localOutBuffer = Mem_Alloc(size);

#ifdef notdef
    /* 
     * These statistics, the overall packet loss and retries, is
     * computable by the user program using the Rpc_Stat system call.
     */
    Rpc_StartSrvTrace();
    Rpc_EnterProcess(0);      /* for tracing */
#endif notdef
 
    if (deltaTimePtr == (Time *)NIL) {
	Sys_Printf("Echoing %d %d-byte messages\n", numEchoes, size);
    }
                 
    Timer_GetCurrentTicks(&startTime);
    packetId = 0;
    do {
        packetId++;
        status = Rpc_Echo(serverId, localInBuffer, localOutBuffer, size);
    } while ((status == SUCCESS) && (packetId < numEchoes));
 
    Timer_GetCurrentTicks(&endTime);
 
    if ((deltaTimePtr == (Time *)NIL) && (status != SUCCESS)) {
        Sys_SafePrintf("got error %x from Rpc_Echo\n", status);
    }

    /*
     * Compute time per RPC.
     */
    Timer_SubtractTicks(endTime, startTime, &endTime);
    Timer_TicksToTime(endTime, &diff);
    Time_Divide(diff, numEchoes, &diff);
    if (deltaTimePtr == (Time *)NIL) {
	Sys_SafePrintf("time per RPC %d.%06d\n",
                        diff.seconds, diff.microseconds);
    } else {
	*deltaTimePtr = diff;
    }
/*
 * Hack alert. Cache the last value of delta time for the RPC's.
 */
    rpcDeltaTime = diff;

#ifdef notdef
    Rpc_LeaveProcess(0);      /* for tracing */               
    Rpc_EndSrvTrace();
#endif notdef

    Byte_Copy(size, localOutBuffer, returnPtr);
    Mem_Free(localOutBuffer);
    Mem_Free(localInBuffer);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Rpc_SendTest --
 *
 *	Send a bunch of packets to a server and time it.
 *
 * Results:
 *	The status code from the RPC.
 *
 * Side effects:
 *	Send off packets to the server.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Rpc_SendTest(serverId, numSends, size, inputPtr, deltaTimePtr)
    int serverId;
    int numSends;
    int size;
    Address inputPtr;
    Time *deltaTimePtr;	/* Return: the average time per RPC.  If this is
			 * a NIL pointer then the results are printed to
			 * the console instead. */
{
    int			packetId;
    ReturnStatus	status;
    Timer_Ticks		startTime;
    Timer_Ticks		endTime;
    Time		diff;
    Address		localInBuffer;

    localInBuffer = Mem_Alloc(size);
    Byte_Copy(size, inputPtr, localInBuffer);

#ifdef notdef
    Rpc_StartSrvTrace();
    Rpc_EnterProcess(0);      /* for tracing */
#endif notdef
 
    if (deltaTimePtr == (Time *)NIL) {
	Sys_Printf("Sending %d %d-byte messages\n", numSends, size);
    }
                 
    Timer_GetCurrentTicks(&startTime);
    packetId = 0;
    do {
        packetId++;
        status = Rpc_Send(serverId, localInBuffer, size);
    } while ((status == SUCCESS) && (packetId < numSends));
 
    Timer_GetCurrentTicks(&endTime);
 
    if ((deltaTimePtr == (Time *)NIL) && (status != SUCCESS)) {
        Sys_SafePrintf("got error %x from Rpc_Send\n", status);
    }

    /*
     * Compute time per RPC.
     */
    Timer_SubtractTicks(endTime, startTime, &endTime);
    Timer_TicksToTime(endTime, &diff);
    Time_Divide(diff, numSends, &diff);
    if (deltaTimePtr == (Time *)NIL) {
	Sys_SafePrintf("time per RPC %d.%06d\n",
                        diff.seconds, diff.microseconds);
    } else {
	*deltaTimePtr = diff;
    }
/*
 * Hack alert. Cache the last value of delta time for the RPC's.
 */
    rpcDeltaTime = diff;
#ifdef notdef
    Rpc_LeaveProcess(0);      /* for tracing */               
    Rpc_EndSrvTrace();
#endif notdef

    Mem_Free(localInBuffer);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Rpc_Echo --
 *
 *	Bounce data off of the specified server.
 *
 * Results:
 *	The status code from the RPC.
 *
 * Side effects:
 *	If the RPC is successful the input data is copied into
 *	the return data.  This is an expensive copy...
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Rpc_Echo(serverId, inputPtr, returnPtr, size)
    int serverId;
    Address inputPtr;
    Address returnPtr;
    int size;
{
    Rpc_Storage storage;
    ReturnStatus status;

    storage.requestDataPtr = inputPtr;
    storage.requestDataSize = size;

    storage.replyDataPtr = returnPtr;
    storage.replyDataSize = size;

    storage.replyParamPtr = (Address)NIL;
    storage.replyParamSize = 0;

    storage.requestParamPtr = (Address)NIL;
    storage.requestParamSize = 0;

    status = Rpc_Call(serverId, RPC_ECHO_2, &storage);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Rpc_Ping --
 *
 *	Ping a remote host by doing a low level echo RPC.  The RPC
 *	is handled at interrupt level by the remote host for minimal impact.
 *
 * Results:
 *	The status code from the RPC.
 *
 * Side effects:
 *	If the RPC is successful the input data is copied into
 *	the return data.  This is an expensive copy...
 *
