/*
 * rpcSrvStat.c --
 *      Manipulation and printing of the statistics taken on the server
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
#include "rpcSrvStat.h"
#include "rpc.h"
#include "rpcServer.h"

/*
 * Stats are taken during RPC to help make sure all parts
 * of the algorithm are exersiced and to monitor the condition
 * of the system.
 * Two sets of statistics are kept, a total and a triptik.
 */
Rpc_SrvStat rpcTotalSrvStat;
Rpc_SrvStat rpcSrvStat;
static int numStats = sizeof(Rpc_SrvStat) / sizeof(int);

#ifdef notdef
/*
 * This is the monitored data whichs keeps track of how many processes
 * are using the RPC system.
 */
static int numTracedRpcServers;

/*
 * The entering and leaving monitored.
 */
static Sync_Lock rpcSrvTraceLock = Sync_LockInitStatic("Rpc:rpcSrvTraceLock");
#define LOCKPTR (&rpcSrvTraceLock)
#endif /* notdef */

static void SpecialSrvStat _ARGS_((int packetLength, int expectedLength));
static void SpecialSrvStatReset _ARGS_((void));
static void SpecialSrvStatPrint _ARGS_((void));



/*
 *----------------------------------------------------------------------
 *
 * Rpc_StartSrvTrace --
 *
 *      Start tracing of server statistics  This call should be followed
 *      later by a call to Rpc_EndSrvTrace and then Rpc_PrintSrvTrace.
 *      These procedures are used to start, stop, and print statistics on
 *      the server side of the RPC system.
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
ENTRY void
Rpc_StartSrvTrace()
{
    LOCK_MONITOR;


    numTracedRpcServers++;
    if (numTracedRpcServers == 1) {
	RpcResetSrvStat();
    }

    UNLOCK_MONITOR;
}
#endif /* notdef */

/*
 *----------------------------------------------------------------------
 *
 * RpcResetSrvStat --
 *
 *	Accumulate the server side stats in the Totals struct and
 *	reset the current counters.  This is not synchronized with
 *	interrupt time code so errors may occur.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Increment the counters in the Total struct and reset the
 *	current counters to zero.
 *
 *----------------------------------------------------------------------
 */
void
RpcResetSrvStat()
{
    register int *totalIntPtr;
    register int *deltaIntPtr;
    register int index;
    /*
     * Add the current statistics to the totals and then
     * reset the counters.  The statistic structs are cast
     * into integer arrays to make this easier to maintain.
     */
    totalIntPtr = (int *)&rpcTotalSrvStat;
    deltaIntPtr = (int *)&rpcSrvStat;
    for (index = 0; index<numStats ; index++) {
	*totalIntPtr += *deltaIntPtr;
	totalIntPtr++;
	deltaIntPtr++;
    }
    bzero((Address)&rpcSrvStat, sizeof(Rpc_SrvStat));

    SpecialSrvStatReset();
}

/*
 *----------------------------------------------------------------------
 *
 * Rpc_EndSrvTrace --
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
ENTRY void
Rpc_EndSrvTrace()
{
    LOCK_MONITOR;

    numTracedRpcServers--;
    if (numTracedRpcServers <= 0) {
	numTracedRpcServers = 0;

	Rpc_PrintSrvStat();
    }

    UNLOCK_MONITOR;
}
#endif /* notdef */
/*
 *----------------------------------------------------------------------
 *
 * Rpc_PrintSrvStat --
 *
 *	Print the RPC server statistics.
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
Rpc_PrintSrvStat()
{
    printf("Rpc Server Statistics\n");
    printf("toServer   = %5d ", rpcSrvStat.toServer);
    printf("noAlloc     = %4d ", rpcSrvStat.noAlloc);
    printf("nacks       = %4d ", rpcSrvStat.nacks);
    printf("invClient   = %4d ", rpcSrvStat.invClient);
    printf("\n");
    printf("serverBusy  = %4d ", rpcSrvStat.serverBusy);
    printf("requests   = %5d ", rpcSrvStat.requests);
    printf("impAcks    = %5d ", rpcSrvStat.impAcks);
    printf("handoffs   = %5d ", rpcSrvStat.handoffs);
    printf("\n");
    printf("fragMsgs   = %5d ", rpcSrvStat.fragMsgs);
    printf("handoffAcks = %4d ", rpcSrvStat.handoffAcks);
    printf("fragAcks    = %4d ", rpcSrvStat.fragAcks);
    printf("sentPartial = %4d ", rpcSrvStat.recvPartial);
    printf("\n");
    printf("busyAcks    = %4d ", rpcSrvStat.busyAcks);
    printf("resends     = %4d ", rpcSrvStat.resends);
    printf("badState    = %4d ", rpcSrvStat.badState);
    printf("extra       = %4d ", rpcSrvStat.extra);
    printf("\n");
    printf("reclaims    = %4d ", rpcSrvStat.reclaims);
    printf("reassembly = %5d ", rpcSrvStat.reassembly);
    printf("dupFrag     = %4d ", rpcSrvStat.dupFrag);
    printf("nonFrag     = %4d ", rpcSrvStat.nonFrag);
    printf("\n");
    printf("fragAborts  = %4d ", rpcSrvStat.fragAborts);
    printf("recvPartial = %4d ", rpcSrvStat.recvPartial);
    printf("closeAcks   = %4d ", rpcSrvStat.closeAcks);
    printf("discards    = %4d ", rpcSrvStat.discards);
    printf("\n");
    printf("unknownAcks = %4d ", rpcSrvStat.unknownAcks);
    printf("\n");

    SpecialSrvStatPrint();
}

/*
 *----------------------------------------------------------------------
 *
 * Rpc_PrintServiceCount --
 *
 *	Print the RPC service call counts.
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
Rpc_PrintServiceCount()
{
    register int call;

    printf("Rpc Service Calls\n");
    for (call=0 ; call<=RPC_LAST_COMMAND ; call++) {
	printf("%-15s %8d\n", rpcService[call].name, rpcServiceCount[call]);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * SpecialSrvStat --
 *
 *	Generic tracing hook.  This procedure gets changed to trace
 *	different events.  This hides the details of the statistics
 *	taking from the caller.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	rpcSrvStat.longs is getting incremented.  This is a trace of
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
} specialSrvStat;

static void
SpecialSrvStat(packetLength, expectedLength)
    int packetLength, expectedLength;
{
    specialSrvStat.hits++;
    specialSrvStat.sumLength += packetLength;
    specialSrvStat.lastLength = packetLength;
    specialSrvStat.sumExpLength += expectedLength;
    specialSrvStat.lastExpLength = expectedLength;
}

static void
SpecialSrvStatReset()
{
    specialSrvStat.hits = 0;
    specialSrvStat.sumLength = 0;
    specialSrvStat.lastLength = 0;
    specialSrvStat.sumExpLength = 0;
    specialSrvStat.lastExpLength = 0;
}

static void
SpecialSrvStatPrint()
{
    if (specialSrvStat.hits) {
	printf("Number of Special Stats: %d\n", specialSrvStat.hits);

	printf("Last packet length (%d), last expected length (%d)\n",
			     specialSrvStat.lastLength, specialSrvStat.lastExpLength);

	printf("Ave packet length (%d), ave expected length (%d)\n",
	    (specialSrvStat.sumLength / specialSrvStat.hits),
	    (specialSrvStat.sumExpLength / specialSrvStat.hits));
    }
}
