/*
 * rpcCltStat.c --
 *      Manipulation and printing of the statistics taken on the client
 *      side of the RPC system.  The statistics are kept as simple event
 *      counts.  The counts are incremented in unsynchronized sections of
 *      code.  They are reset and printed out with a pair of synchronized
 *      routines.  Clients of the RPC system can use these to trace long
 *      term RPC exersices.  At any time an RPC client can declare itself
 *      as entering the RPC system for tracing purposes.  Any number of
 *      processes can enter the system for tracing.  After the last
 *      process has left the tracing system the statistics are printed on
 *      the console and then reset.  (There should be a routine that
 *      forces a printout of the statistics... If one process messes up
 *      and doesn't leave then the stats won't get printed.)
 *
 *	Also, there is one special tracing hook used internally by the RPC
 *	system to trace unusual events.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */


#include "sprite.h"
#include "sync.h"
#include "byte.h"
#include "rpcCltStat.h"
#include "user/rpc.h"

/*
 * Stats are taken during RPC to help make sure all parts
 * of the algorithm are exersiced and to monitor the condition
 * of the system.
 * Two sets of statistics are kept, a total and a triptik.
 */
Rpc_CltStat rpcTotalCltStat;
Rpc_CltStat rpcCltStat;
static int numStats = sizeof(Rpc_CltStat) / sizeof(int);

#ifdef notdef
/*
 * This is the monitored data whichs keeps track of how many processes
 * are using the RPC system.
 */
static int numRpcProcesses;

/*
 * The entering and leaving monitored.
 */
static Sync_Lock rpcTraceLock;
#define LOCKPTR (&rpcTraceLock)
#endif /* notdef */

/*
 *----------------------------------------------------------------------
 *
 * Rpc_EnterProcess --
 *
 *      Note that a process is entering the RPC system for tracing.  This
 *      call should be followed later by a call to Rpc_LeaveProcess.
 *      These two procedures are used to start, stop, and print statistics
 *      on the RPC system.  After a process enters, the system waits until
 *      everyone that enters has left and then prints out the accumulated
 *      statistics for the period when the first process registered and
 *      the last process left.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Increment the number of processes in the RPC system, initialize
 *	the statistics structre at the entry of the first process.
 *
 *----------------------------------------------------------------------
 */
#ifdef notdef
void
Rpc_EnterProcess()
{
    LOCK_MONITOR;

    numRpcProcesses++;
    if (numRpcProcesses == 1) {
	RpcResetCltStat();
    }

    UNLOCK_MONITOR;
}
#endif /* notdef */

/*
 *----------------------------------------------------------------------
 *
 * RpcResetCltStat --
 *
 *      Accumulate the client side stats in the Totals struct and
 *      reset the current counters.  This is not synchronized with
 *      interrupt time code so errors may occur.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Increment the counters in the Total struct and reset the
 *      current counters to zero.
 *----------------------------------------------------------------------
 */
void
RpcResetCltStat()
    /*
     * Could be parameterized and combined with RpcResetSrvStat...
     */
{
    register int *totalIntPtr;
    register int *deltaIntPtr;
    register int index;
    /*
     * Add the current statistics to the totals and then
     * reset the counters.  The statistic structs are cast
     * into integer arrays to make this easier to maintain.
     */
    totalIntPtr = (int *)&rpcTotalCltStat;
    deltaIntPtr = (int *)&rpcCltStat;
    for (index = 0; index<numStats ; index++) {
        *totalIntPtr += *deltaIntPtr;
        totalIntPtr++;
        deltaIntPtr++;
    }
    Byte_Zero(sizeof(Rpc_CltStat), (Address)&rpcCltStat);

    RpcSpecialStatReset();
}

/*
 *----------------------------------------------------------------------
 *
 * Rpc_LeaveProcess --
 *
 *	Note that a process has left the RPC system for tracing.
 *	After the last process leaves the RPC system this prints out the
 *	statistics that have accrued so far.
 *
 * Results:
 *	Maybe to the printfs.
 *
 * Side effects:
 *	Decrement the number of processes in the RPC system.
 *
 *----------------------------------------------------------------------
 */
#ifdef notdef
void
Rpc_LeaveProcess(pid)
    int pid;
{
    LOCK_MONITOR;

    numRpcProcesses--;
    if (numRpcProcesses <= 0) {
	numRpcProcesses = 0;

	Rpc_PrintCltStat();

    }

    UNLOCK_MONITOR;
}
#endif /* notdef */

/*
 *----------------------------------------------------------------------
 *
 * Rpc_PrintCltStat --
 *
 *	Print the client RPC statistics structure.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Do the prints.
 *
 *----------------------------------------------------------------------
 */
void
Rpc_PrintCltStat()
{
    Sys_Printf("Rpc Statistics\n");
    Sys_Printf("toClient   = %5d ", rpcCltStat.toClient);
    Sys_Printf("badChannel  = %4d ", rpcCltStat.badChannel);
    Sys_Printf("chanBusy    = %4d ", rpcCltStat.chanBusy);
    Sys_Printf("badId       = %4d ", rpcCltStat.badId);
    Sys_Printf("\n");
    Sys_Printf("requests   = %5d ", rpcCltStat.requests);
    Sys_Printf("replies    = %5d ", rpcCltStat.replies);
    Sys_Printf("acks        = %4d ", rpcCltStat.acks);
    Sys_Printf("recvPartial = %4d ", rpcCltStat.recvPartial);
    Sys_Printf("\n");
    Sys_Printf("timeouts    = %4d ", rpcCltStat.timeouts);
    Sys_Printf("aborts      = %4d ", rpcCltStat.aborts);
    Sys_Printf("resends     = %4d ", rpcCltStat.resends);
    Sys_Printf("sentPartial = %4d ", rpcCltStat.sentPartial);
    Sys_Printf("\n");
    Sys_Printf("errors      = %d(%d)", rpcCltStat.errors,
				       rpcCltStat.nullErrors);
    Sys_Printf("dupFrag     = %4d ", rpcCltStat.dupFrag);
    Sys_Printf("close       = %4d ", rpcCltStat.close);
    Sys_Printf("\n");
    Sys_Printf("oldInputs   = %4d ", rpcCltStat.oldInputs);
    Sys_Printf("badInputs   = %4d ", rpcCltStat.oldInputs);
    Sys_Printf("tooManyAcks = %4d ", rpcCltStat.tooManyAcks);
    Sys_Printf("\n");
    Sys_Printf("chanHits   = %5d ", rpcCltStat.chanHits);
    Sys_Printf("chanNew     = %4d ", rpcCltStat.chanNew);
    Sys_Printf("chanReuse   = %4d ", rpcCltStat.chanReuse);
    Sys_Printf("\n");
    Sys_Printf("chanWaits   = %4d ", rpcCltStat.chanWaits);
    Sys_Printf("chanBroads  = %4d ", rpcCltStat.chanBroads);
    Sys_Printf("\n");
    Sys_Printf("paramOverrun = %3d ", rpcCltStat.paramOverrun);
    Sys_Printf("dataOverrun = %4d ", rpcCltStat.dataOverrun);
    Sys_Printf("shorts      = %4d ", rpcCltStat.shorts);
    Sys_Printf("longs       = %4d ", rpcCltStat.longs);
    Sys_Printf("\n");

    RpcSpecialStatPrint();
}

/*
 *----------------------------------------------------------------------
 *
 * RpcSpecialStat --
 *
 *	Generic tracing hook.  This procedure gets changed to trace
 *	different events.  This hides the details of the statistics
 *	taking from the caller.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	rpcCltStat.longs is getting incremented.  This is a trace of
 *	the expected packet size and the actual packet size to
 *	try and see what is happening.
 *
 *----------------------------------------------------------------------
 */

static struct {
    int		hits;
    int		sumLength;
    int		lastLength;
    int		sumExpLength;
    int		lastExpLength;
} specialStat;

RpcSpecialStat(packetLength, expectedLength)
    int packetLength, expectedLength;
{
    specialStat.hits++;
    specialStat.sumLength += packetLength;
    specialStat.lastLength = packetLength;
    specialStat.sumExpLength += expectedLength;
    specialStat.lastExpLength = expectedLength;
}

RpcSpecialStatReset()
{
    specialStat.hits = 0;
    specialStat.sumLength = 0;
    specialStat.lastLength = 0;
    specialStat.sumExpLength = 0;
    specialStat.lastExpLength = 0;
}

RpcSpecialStatPrint()
{
    if (specialStat.hits) {
	Sys_SafePrintf("Number of Special Stats: %d\n", specialStat.hits);

	Sys_SafePrintf("Last packet length (%d), last expected length (%d)\n",
			     specialStat.lastLength, specialStat.lastExpLength);

	Sys_SafePrintf("Ave packet length (%d), ave expected length (%d)\n",
	    (specialStat.sumLength / specialStat.hits),
	    (specialStat.sumExpLength / specialStat.hits));
    }
}
