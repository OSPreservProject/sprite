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
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */


#include <sprite.h>
#include <stdio.h>
#include <bstring.h>
#include <sync.h>
#include <rpcCltStat.h>
#include <user/rpc.h>
#include <rpcServer.h>
#include <rpc.h>
#include <rpcClient.h>

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
static Sync_Lock rpcTraceLock = Sync_LockInitStatic("Rpc:rpcTraceLock");
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
ENTRY void
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
    bzero((Address)&rpcCltStat, sizeof(Rpc_CltStat));
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
ENTRY void
Rpc_LeaveProcess()
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
    printf("Rpc Statistics\n");
    printf("toClient   = %5d ", rpcCltStat.toClient);
    printf("badChannel  = %4d ", rpcCltStat.badChannel);
    printf("chanBusy    = %4d ", rpcCltStat.chanBusy);
    printf("badId       = %4d ", rpcCltStat.badId);
    printf("\n");
    printf("requests   = %5d ", rpcCltStat.requests);
    printf("replies    = %5d ", rpcCltStat.replies);
    printf("acks        = %4d ", rpcCltStat.acks);
    printf("recvPartial = %4d ", rpcCltStat.recvPartial);
    printf("\n");
    printf("nacks       = %4d ", rpcCltStat.nacks);
    printf("reNacks	= %4d ", rpcCltStat.reNacks);
    printf("maxNacks	= %4d ", rpcCltStat.maxNacks);
    printf("timeouts    = %4d ", rpcCltStat.timeouts);
    printf("\n");
    printf("aborts      = %4d ", rpcCltStat.aborts);
    printf("resends     = %4d ", rpcCltStat.resends);
    printf("sentPartial = %4d ", rpcCltStat.sentPartial);
    printf("errors      = %d(%d)", rpcCltStat.errors,
				       rpcCltStat.nullErrors);
    printf("\n");
    printf("dupFrag     = %4d ", rpcCltStat.dupFrag);
    printf("close       = %4d ", rpcCltStat.close);
    printf("oldInputs   = %4d ", rpcCltStat.oldInputs);
    printf("badInputs   = %4d ", rpcCltStat.oldInputs);
    printf("\n");
    printf("tooManyAcks = %4d ", rpcCltStat.tooManyAcks);
    printf("chanHits   = %5d ", rpcCltStat.chanHits);
    printf("chanNew     = %4d ", rpcCltStat.chanNew);
    printf("chanReuse   = %4d ", rpcCltStat.chanReuse);
    printf("\n");
    printf("newTrouble  = %4d ", rpcCltStat.newTrouble);
    printf("moreTrouble = %4d ", rpcCltStat.moreTrouble);
    printf("endTrouble  = %4d ", rpcCltStat.newTrouble);
    printf("noMark      = %4d ", rpcCltStat.noMark);
    printf("\n");
    printf("nackChanWait= %4d ", rpcCltStat.nackChanWait);
    printf("chanWaits   = %4d ", rpcCltStat.chanWaits);
    printf("chanBroads  = %4d ", rpcCltStat.chanBroads);
    printf("paramOverrun = %3d ", rpcCltStat.paramOverrun);
    printf("\n");
    printf("dataOverrun = %4d ", rpcCltStat.dataOverrun);
    printf("shorts      = %4d ", rpcCltStat.shorts);
    printf("longs       = %4d ", rpcCltStat.longs);
    printf("\n");
}

/*
 *----------------------------------------------------------------------
 *
 * Rpc_PrintCallCount --
 *
 *	Print the RPC call counts.
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
Rpc_PrintCallCount()
{
    register int call;

    printf("Rpc Client Calls\n");
    for (call=0 ; call<=RPC_LAST_COMMAND ; call++) {
	printf("%-15s %8d\n", rpcService[call].name, rpcClientCalls[call]);
    }
}
