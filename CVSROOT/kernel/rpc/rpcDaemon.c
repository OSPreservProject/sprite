/* 
 * rpcDaemon.c --
 *
 *	The RPC daemon is in charge of reclaiming server processes,
 *	and creating more if neccessary.  A server process is dedicated
 *	to a particular client/channel for a series of RPCs.  After the
 *	series (as determined by this daemon) the server is reclaimed
 *	and made available to service RPC requests from other clients.
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
#include "rpcServer.h"
#include "sync.h"
#include "proc.h"

/*
 * Server processes are dynamically created by the Rpc_Daemon process.
 * Installation of new servers, their allocation, and control of the
 * Rpc_Daemon process is all synchronized with the serverMutex master lock.
 * rpcDaemon is the condition that is used to wakeup the Rpc_Deamon.
 */
static int serverMutex;
Sync_Condition rpcDaemon;

/*
 * The timeout queue is used to periodically wake up the Rpc_Daemon.
 */
static Timer_QueueElement queueEntry;
/*
 * The state of Rpc_Daemon is used to know when to remove things
 * from the timeout queue, and to lock out attempts at server allocation
 * until the daemon is alive and can create server processes.
 *	DAEMON_DEAD	The initial state of Rpc_Daemon.  This causes
 *		RpcServerAlloc to discard any incomming messages.
 *	DAEMON_TIMEOUT	The daemon has an entry in the timeout queue.
 *	DAEMON_POKED	Set when rpcDaemon condition is notified to
 *		let the daemon know if it woke up spuriously or not.
 */
#define DAEMON_TIMEOUT	1
#define DAEMON_POKED	2
#define DAEMON_DEAD	4
static int daemonState = DAEMON_DEAD;

/*
 * Forward declarations.
 */
void RpcReclaimServers();
void RpcReclaim();
void RpcDaemonWakeup();
void RpcDaemonWait();
void RpcResetNoServers();

/*
 *----------------------------------------------------------------------
 *
 * Rpc_Daemon --
 *
 *	The main loop of the rpcDaemon process.  This process sleeps for
 *	regular intervals and then pokes around looking for RPC server
 *	processes that have been idle for a while.  Idle servers are
 *	reclaimed by tidying up their connection with their old client
 *	and making them available to handle RPC requests from other clients.
 *	The other chore of the daemon is to create more RPC server processes
 *	if the demand for them is high.  Initially a few server process
 *	is created and the rest are created via this daemon.
 *
 * Results:
 *	This never returns.
 *
 * Side effects:
 *	Evolves a server process's state from BUSY to AGING and back to FREE.
 *	Creates new RPC server processes.
 *
 *----------------------------------------------------------------------
 */
void
Rpc_Daemon()
{
    int pid;

    queueEntry.routine = RpcDaemonWakeup;
    queueEntry.interval = 5 * timer_IntOneSecond;
    queueEntry.clientData = (ClientData)NIL;

    Sys_Printf("Rpc_Daemon alive\n");

    while (TRUE) {
	RpcDaemonWait(&queueEntry);
	if (rpcNoServers > 0) {
	    /*
	     * The dispatcher has received requests that it couldn't handle
	     * because there were no available server processes.
	     */
	     Rpc_CreateServer(&pid);
	     Sys_Printf("Created RPC server %x\n", pid);
	     RpcResetNoServers(0);
	}
	RpcReclaimServers();
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Rpc_CreateServer(pidPtr) --
 *
 *      Create an RPC server process.  It won't be available to handle RPC
 *      requests until it has run a bit and installed itself.  This
 *      procedure is not monitored although it is called from the main program
 *      and by the Rpc_Daemon process.  Currently the initialization done
 *	in main is done before Rpc_Deamon begins running.
 *
 * Results:
 *	A status from the process creating call.
 *
 * Side effects:
 *	Create the server process, set up its state table entry, and
 *	return the caller the process ID of the server.  The counter
 *	rpcNumServers is incremented.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Rpc_CreateServer(pidPtr)
    int *pidPtr;
{
    register ReturnStatus status;

    if (rpcNumServers >= rpcMaxServers) {
	return(FAILURE);
    }
    /*
     * Initialize the next slot in the server state table and create a
     * process that goes with it.
     */
    rpcServerPtrPtr[rpcNumServers] = RpcInitServerState(rpcNumServers);
    rpcNumServers++;
    status = Proc_NewProc((Address)Rpc_Server, PROC_KERNEL, FALSE, pidPtr,
			  "Rpc_Server");
    if (status == SUCCESS) {
	Proc_SetServerPriority(*pidPtr);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * RpcServerAlloc --
 *
 *	Match up an incoming message to its server.  The clientID field
 *	of the header is used to identify the client.  This ID has to
 *	already have been validated by RpcValidateClient.
 *
 * Results:
 *	A pointer to the state of a server process.  The server is either
 *	currently involved in a transaction with the client, or it was
 *	the server for the client on the client's last transaction,
 *	or it is newly allocated to the client.
 *
 * Side effects:
 *	If this message is from a different client than the server's
 *	previous client, the server's state is updated to identify
 *	the new client.
 *
 *----------------------------------------------------------------------
 */
RpcServerState *
RpcServerAlloc(rpcHdrPtr)
    RpcHdr *rpcHdrPtr;
{
    register int startIndex;
    register int srvIndex;
    register RpcServerState *srvPtr;
    int	freeServer = -1;

    MASTER_LOCK(serverMutex);

    if (daemonState == DAEMON_DEAD) {
	/*
	 * The RPC system isn't up enough to accept requests.
	 */
	srvPtr = (RpcServerState *)NIL;
	goto unlock;
    }
    /*
     * Start looking at the server indicated by the client's hint.
     */
    srvIndex = rpcHdrPtr->serverHint;
    if (srvIndex < 0 || srvIndex >= rpcNumServers) {
	srvIndex = 0;
    }
    startIndex = srvIndex;
    do {
	srvPtr = rpcServerPtrPtr[srvIndex];
	if (srvPtr != (RpcServerState *)NIL) {
	    if (srvPtr->clientID == rpcHdrPtr->clientID &&
		srvPtr->channel == rpcHdrPtr->channel) {
		srvPtr->state &= ~SRV_FREE;
		goto unlock;
	    } else if ((freeServer == -1) && (srvPtr->state & SRV_FREE)) {
		freeServer = srvIndex;
	    }
	}
	srvIndex = (srvIndex + 1) % rpcNumServers;
    } while (srvIndex != startIndex);

    if (freeServer != -1) {
	/*
	 * Reassigning a free server to a new client.
	 */
	srvPtr = rpcServerPtrPtr[freeServer];
	srvPtr->state &= ~SRV_FREE;
	srvPtr->clientID = rpcHdrPtr->clientID;
	srvPtr->channel = rpcHdrPtr->channel;
    } else {
	/*
	 * No available server process yet.  Poke the Rpc_Daemon process
	 * so it will create us one.  The client's request gets discarded
	 * and it has to re-try.
	 */
	srvPtr = (RpcServerState *)NIL;
/*	Sys_Printf("No free servers! <%d>\n", daemonState);	*/
	rpcNoServers++;
	if (daemonState & DAEMON_TIMEOUT) {
	    Timer_DescheduleRoutine(&queueEntry);
	    daemonState &= ~DAEMON_TIMEOUT;
	}
	daemonState |= DAEMON_POKED;
	Sync_MasterBroadcast(&rpcDaemon);
    }
unlock:
    MASTER_UNLOCK(serverMutex);
    return(srvPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * RpcServerInstall --
 *
 *	Assign a server process to an entry in the server table.
 *	This would be trivial if the server process could be
 *	passed an argument, ie, a pointer to it's state table entry.
 *
 * Results:
 *	A pointer to the state table entry for the server.
 *
 * Side effects:
 *	Change the state of the server from NOTREADY to FREE.
 *
 *----------------------------------------------------------------------
 */
ENTRY RpcServerState *
RpcServerInstall()
{
    RpcServerState *srvPtr;
    register int i;
    /*
     * This synchronizes with RpcServerAlloc.  This will only be
     * important if the server is getting requests as the machine boots.
     */
    MASTER_LOCK(serverMutex);

    for (i=0 ; i<rpcNumServers ; i++) {
	srvPtr = rpcServerPtrPtr[i];
	if (srvPtr->state == SRV_NOTREADY) {
	    srvPtr->state = SRV_FREE;
	    goto unlock;
	}
    }
    srvPtr = (RpcServerState *)NIL;
unlock:
    MASTER_UNLOCK(serverMutex);
    return(srvPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * RpcResetNoServers --
 *
 *	A tiny routine to synchronize access to the rpcNoServers counter.
 *	This counter is used by the server dispatcher to communicate out
 *	to the Rpc_Deamon that there are not enough server processes.
 *	This is checked (unsynchronizedly...) by Rpc_Deamon and then
 *	reset (if non-zero) via this routine.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Reset the rpcNoServers counter to the indicated value.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
RpcResetNoServers(value)
    int value;		/* New value for rpcNoServers */
{
    MASTER_LOCK(serverMutex);
    rpcNoServers = value;
    MASTER_UNLOCK(serverMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * RpcDaemonWait --
 *
 *	Make the Rpc_Daemon process wait.  This has to be synchronized with
 *	the routines that wakeup the daemon.  The serverMutex is used as
 *	the lock for these routines.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Puts the routine in the timeout queue under the protection of
 *	the serverMutex master lock.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
RpcDaemonWait(queueEntryPtr)
    Timer_QueueElement *queueEntryPtr;	/* Initialized timer queue item */
{
    MASTER_LOCK(serverMutex);
    if (daemonState & DAEMON_DEAD) {
	daemonState &= ~DAEMON_DEAD;
    }
    daemonState |= DAEMON_TIMEOUT;
    Timer_ScheduleRoutine(queueEntryPtr, TRUE);
    do {
	Sync_MasterWait(&rpcDaemon, &serverMutex, FALSE);
	if (sys_ShuttingDown) {
	    Sys_Printf("Rpc_Daemon exiting.\n");
	    MASTER_UNLOCK(serverMutex);
	    Proc_Exit(0);
	}
    } while ((daemonState & DAEMON_POKED) == 0);
    daemonState &= ~DAEMON_POKED;
    MASTER_UNLOCK(serverMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * RpcDaemonWakeup --
 *
 *	Called from the timeout queue to wakeup the Rpc_Daemon.  Grabs
 *	the serverMutex lock and notifies rpcDaemon condition to
 *	wakeup the Rpc_Daemon process.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Notifies the rpcDaemon condition.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ENTRY void
RpcDaemonWakeup(time, data)
    Timer_Ticks time;		/* Time we timed out at. */
    ClientData data;		/* NIL */
{
    MASTER_LOCK(serverMutex);
    daemonState &= ~DAEMON_TIMEOUT;
    daemonState |= DAEMON_POKED;
    Sync_MasterBroadcast(&rpcDaemon);
    MASTER_UNLOCK(serverMutex);
}
