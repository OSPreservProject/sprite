/* 
 * rpcTrace.c --
 *
 *	The tracing routines for the Rpc system.  As messages are moved
 *	through the RPC system time-stamped trace records can be taken
 *	to see where the system spends its time.  The circular buffer
 *	of trace records can be dumped to a file or printed to the console.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "rpc.h"
#include "rpcInt.h"
#include "rpcTrace.h"
#include "rpcServer.h"
#include "net.h"
#include "status.h"
#include "dbg.h"
#include "machine.h"
#include "fs.h"
#include "timer.h"
#include "byte.h"
#include "string.h"

/*
 * The circular buffer of trace records.
 */
#ifndef CLEAN
Boolean		rpc_Tracing = TRUE;	/* flag to turn on tracing */
#else
Boolean		rpc_Tracing = FALSE;	/* No tracing in clean version */
#endif not CLEAN
Trace_Header	rpcTraceHdr;		/* Trace header info */
Trace_Header	*rpcTraceHdrPtr = &rpcTraceHdr;
/*
 * The results of some standard benchmarks are cached so they
 * get printed out with the trace records.
 */
Time rpcDeltaTime;		/* Average time per RPC */
Time rpcEmptyStampTime;		/* The time to take a trace record without
				 * copying the packet header */
Time rpcFullStampTime;		/* The time to take a trace record that
				 * includes copying the packet header */

/*
 *----------------------------------------------------------------------
 *
 * Rpc_PrintTrace --
 *
 *	Print out the last few trace records.  Can be called from
 *	 the debugger or via the Test_Stat system call.
 *
 * Results:
 *	Print out the trace records on the console.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
Rpc_PrintTrace(numRecords)
    int numRecords;	/* The number (of most recent) records to print */
{
    register int i;	/* Index into trace table */
    int stopIndex;	/* copy of rpcTraceIndex */
    Time baseTime, deltaTime;	/* Times for print out */
    Trace_Record *recordPtr;
    RpcHdr	*rpcHdrPtr;
    int c;		/* Used to identify record types in output */
    char flagString[8];	/* Used to format rpc header flags */
    int stringIndex;

    rpcTraceHdrPtr->inhibitTracing = TRUE;
    if (numRecords > rpcTraceHdrPtr->numRecords) {
	numRecords = rpcTraceHdrPtr->numRecords;
    }
    /*
     * Start the print numRecords before rpcTraceIndex.
     * We make a copy of rpcTraceIndex in case in gets incremented
     * during our set of print statements.
     */
    i = (rpcTraceHdrPtr->currentRecord + RPC_TRACE_LEN - numRecords)
		% RPC_TRACE_LEN;
    stopIndex = rpcTraceHdrPtr->currentRecord;

    Sys_Printf("\n");
#define PRINT_HEADER() \
    Sys_Printf("%8s %4s %9s %5s %4s %4s %10s %5s %5s %5s %8s\n", \
	"ID", "type", "time", "flags", "srvr", "clnt", "cmd   ", \
	"psize", "dsize", "doff", "fragInfo")
    PRINT_HEADER();

    baseTime.seconds = 0;
    baseTime.microseconds = 0;
    do {
	recordPtr = &rpcTraceHdrPtr->recordArray[i];
	Time_Subtract(recordPtr->time, baseTime, &deltaTime);

	rpcHdrPtr = (RpcHdr *)recordPtr->traceData;
	Sys_Printf("%8x ", rpcHdrPtr->ID);

	switch(recordPtr->event) {
	    default: 
		Sys_Printf("{%d}", recordPtr->event);
		break;
	    case RPC_INPUT:
		Sys_Printf("  <");
		break;
	    case RPC_ETHER_OUT:
		Sys_Printf(">  ");
		break;
	    case RPC_OUTPUT:
		Sys_Printf(" > ");
		break;
	    case RPC_CLIENT_a:		/* Client interrupt time stamps */
	    case RPC_CLIENT_b:
	    case RPC_CLIENT_c:
	    case RPC_CLIENT_d:
	    case RPC_CLIENT_e:
	    case RPC_CLIENT_f:
		c = recordPtr->event - RPC_CLIENT_a + 'a';
		Sys_Printf("Ci%c", c);
		break;
	    case RPC_CLIENT_A:		/* Client process level time stamps */
	    case RPC_CLIENT_B:
	    case RPC_CLIENT_C:
	    case RPC_CLIENT_D:
	    case RPC_CLIENT_E:
	    case RPC_CLIENT_F:
		c = recordPtr->event - RPC_CLIENT_A + 'A';
		Sys_Printf("Cp%c", c);
		break;
	    case RPC_SERVER_a:		/* Server interrupt time stamps */
	    case RPC_SERVER_b:
	    case RPC_SERVER_c:
	    case RPC_SERVER_d:
	    case RPC_SERVER_e:
	    case RPC_SERVER_f:
		c = recordPtr->event - RPC_SERVER_a + 'a';
		Sys_Printf("Si%c", c);
		break;
	    case RPC_SERVER_A:		/* Server process level time stamps */
	    case RPC_SERVER_B:
	    case RPC_SERVER_C:
	    case RPC_SERVER_D:
	    case RPC_SERVER_E:
	    case RPC_SERVER_F:
		c = recordPtr->event - RPC_SERVER_A + 'A';
		Sys_Printf("Sp%c", c);
		break;
	    case RPC_CLIENT_OUT:
		Sys_Printf("Cx ");
		break;
	    case RPC_SERVER_OUT:
		Sys_Printf("Sx ");
		break;
	}

	Sys_Printf(" %2d.%06d",
			   deltaTime.seconds,
			   deltaTime.microseconds);
	baseTime = recordPtr->time;
	switch(rpcHdrPtr->flags & RPC_TYPE) {
	    case RPC_REQUEST:
		flagString[0] = 'Q';
		break;
	    case RPC_REPLY:
		flagString[0] = 'R';
		break;
	    case RPC_ACK:
		flagString[0] = 'A';
		break;
	    default:
		flagString[0] = '-';
	}
	stringIndex = 1;
	if (rpcHdrPtr->flags & RPC_PLSACK) {
	    flagString[stringIndex++] = 'p';
	}
	if (rpcHdrPtr->flags & RPC_LASTFRAG) {
	    flagString[stringIndex++] = 'f';
	}
	if (rpcHdrPtr->flags & RPC_CLOSE) {
	    flagString[stringIndex++] = 'c';
	}
	if (rpcHdrPtr->flags & RPC_ERROR) {
	    flagString[stringIndex++] = 'e';
	}
	flagString[stringIndex] = '\0';
	Sys_Printf(" %5s", flagString);
	Sys_Printf(" %4d %4d ",
			   rpcHdrPtr->serverID,
			   rpcHdrPtr->clientID);
	if (rpcHdrPtr->command >= 0 && rpcHdrPtr->command <= RPC_LAST_COMMAND) {
	    Sys_Printf("%10s", rpcService[rpcHdrPtr->command].name);
	} else {
	    Sys_Printf("%10d", rpcHdrPtr->command);
	}
	Sys_Printf(" %5d %5d %5d %2d %2x %5d",
			   rpcHdrPtr->paramSize,
			   rpcHdrPtr->dataSize,
			   rpcHdrPtr->dataOffset,
			   rpcHdrPtr->numFrags,
			   rpcHdrPtr->fragMask,
			   rpcHdrPtr->delay);
	Sys_Printf("\n");

	i = (i + 1) % RPC_TRACE_LEN;
    } while (i != stopIndex);
    PRINT_HEADER();
    Sys_Printf("Delta time = %6d.%06d\n", rpcDeltaTime.seconds,
			  rpcDeltaTime.microseconds);
    rpcTraceHdrPtr->inhibitTracing = FALSE;
}

/*
 *----------------------------------------------------------------------
 *
 * Rpc_DumpTrace --
 *
 *	Dump out trace records to a file.  Can be called from
 *	 the debugger or via the Test_Stat system call.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Dump out the trace records to the file.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Rpc_DumpTrace(firstRec, lastRec, fileName)
    int firstRec;	/* The index of the first record to print.  The
			 * numbering is relative to the oldest record with
			 * the oldest record = 1 and the newest record =
			 * RPC_TRACE_LEN */
    int lastRec;	/* The index of the last record to print. */
    char *fileName;	/* Name of the file to which to write */
{
    register int i;		/* Index into trace table */
    int stopIndex;		/* copy of rpcTraceIndex */
    int offset;			/* file offset */
    Fs_Stream *streamPtr;	/* File to write to */
    ReturnStatus status;	/* Return code */
    Rpc_TraceFileHdr rpcFileHdr;/* Trace file header */
    Rpc_Trace	traceRecord;	/* Trace record for output file */
    Trace_Record	*recordPtr;
    int writeLen;

    rpcTraceHdrPtr->inhibitTracing = TRUE;

    status = Fs_Open(fileName, FS_CREATE|FS_WRITE, FS_FILE, 0666, &streamPtr);
    if (status != SUCCESS) {
	goto exit;
    }
    /*
     * Set up the header.
     */
    rpcFileHdr.version = RPC_TRACE_VERSION;
    rpcFileHdr.numRecords = lastRec - firstRec + 1;
    rpcFileHdr.rpcDeltaTime = rpcDeltaTime;
    rpcFileHdr.emptyStampMicroseconds = rpcEmptyStampTime.microseconds;
    rpcFileHdr.fullStampMicroseconds = rpcFullStampTime.microseconds;

    offset = 0;
    writeLen = sizeof(Rpc_TraceFileHdr);
    status = Fs_Write(streamPtr, (Address)&rpcTraceHdr, offset, &writeLen);
    if (status != SUCCESS) {
	(void)Fs_Close(streamPtr);
	goto exit;
    }
    offset += writeLen;
    
    /*
     * rpcTraceIndex points to record #1.  Map from record number to index.
     */
    i = (rpcTraceHdrPtr->currentRecord + firstRec - 1) % RPC_TRACE_LEN;
    stopIndex = (rpcTraceHdrPtr->currentRecord + lastRec) % RPC_TRACE_LEN;

    do {
	writeLen = sizeof(Rpc_Trace);
	recordPtr = &rpcTraceHdrPtr->recordArray[i];
	traceRecord.time = recordPtr->time;
	traceRecord.type = recordPtr->event;
	Byte_Copy(sizeof(RpcHdr), (Address)recordPtr->traceData,
				  (Address)&traceRecord.rpcHdr);
	status = Fs_Write(streamPtr, (Address)&traceRecord,
				     streamPtr->offset, &writeLen);
	if (status != SUCCESS) {
	    (void)Fs_Close(streamPtr);
	    goto exit;
	}
	i = (i + 1) % RPC_TRACE_LEN;
    } while (i != stopIndex);

    (void)Fs_Close(streamPtr);
    status = SUCCESS;
exit:
    rpcTraceHdrPtr->inhibitTracing = FALSE;
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Rpc_StampTest --
 *
 *	This does a calibration timing of the RpcTrace routine.
 *	A numer of calls are made to this routine and the time per
 *	call is computed and printed onto the console.
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
Rpc_StampTest()
{
    int i;
    Timer_Ticks startTime;
    Timer_Ticks endTime;
    Time diff;
    RpcHdr junkRpcHdr;

#define NUMTIMES	1000

    Sys_Printf("RpcTrace timing:  ");
    junkRpcHdr.flags = 0;

    Timer_GetCurrentTicks(&startTime);
     for (i=0 ; i<NUMTIMES ; i++) {
	RpcTrace((RpcHdr *)NIL, RPC_SERVER_OUT, "empty");
     }
    Timer_GetCurrentTicks(&endTime);

    Timer_SubtractTicks(endTime, startTime, &endTime);
    Timer_TicksToTime(endTime, &diff);
    Time_Divide(diff, NUMTIMES, &diff);
    Sys_Printf("empty = %d, ", diff.microseconds);
    rpcEmptyStampTime = diff;


    Timer_GetCurrentTicks(&startTime);
     for (i=0 ; i<NUMTIMES ; i++) {
	RpcTrace(&junkRpcHdr, RPC_CLIENT_OUT, "full");
     }
    Timer_GetCurrentTicks(&endTime);

    Timer_SubtractTicks(endTime, startTime, &endTime);
    Timer_TicksToTime(endTime, &diff);
    Time_Divide(diff, NUMTIMES, &diff);
    Sys_Printf("full = %d usecs\n", diff.microseconds);
    rpcFullStampTime = diff;

    for (i=0 ; i<4 ; i++) {
	RpcTrace(&junkRpcHdr, RPC_SERVER_OUT, "full");
	RpcTrace((RpcHdr *)NIL, RPC_CLIENT_OUT, "empty");
    }
}
