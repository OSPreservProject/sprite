/* 
 * recovery.c --
 *
 *	The routines here maintain up/down state about other hosts.
 *	Other modules register as clients of the recovery module,
 *	and can then ask to be called back when some other host crashes
 *	or reboots.  Modules always get called back when someone crashes,
 *	and then they have the option of being called back when the
 *	host reboots.  Regular message traffic plus explicit pinging
 *	are used to track the state of the other hosts.  Pinging is
 *	only done if some module is explicitly interested in a host.
 *
 *	Recov_HostAlive and Recov_HostDead are used by RPC to tell us when
 *	a messages have arrived, or if transactions have timed out.
 *	Recov_IsHostDown is used to query the state of another host,
 *	Recov_RebootCallBack is used to get a callback upon a reboot, and
 *	Recov_WaitForHost is used to block a process until a host reboots.
 *	(Recov_WaitForHost isn't used much.  Instead, modules rely on the
 *	recovery callbacks to indicate that a host is back to life, and
 *	they block processes in their own way.)
 *
 *	Note: A synchronization hook is provided by Recov_HostAlive;  its
 *	caller can be blocked if crash recovery actions are in progress.
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */


#include "sprite.h"
#include "recov.h"
#include "sync.h"
#include "net.h"
#include "rpc.h"
#include "hash.h"
#include "stdlib.h"
#include "trace.h"
#include "fsutil.h"
#include "bstring.h"

/*
 * Other kernel modules arrange call-backs when a host crashes or reboots.
 * The following list structure is used to keep these.  The calling
 * sequence of the callbacks is as follows:
 *	(*proc)(spriteID, clientData)
 * Use Recov_CrashRegister and Recov_RebootRegister to set up the call backs.
 */

typedef struct {
    List_Links	links;
    void	(*proc)();
    int		refCount;
    ClientData	data;
} NotifyElement;

/*
 * There is a single list of crash call backs, it isn't per machine
 * like the reboot callbacks.
 */
static List_Links	crashCallBackList;

/*
 * recov_CrashDelay is the grace period given when another host
 * is apparently down.  Reboots are still detected so that
 * the crash callbacks will get called to clean up.
 */
unsigned int recov_CrashDelay;

/*
 * Statistics about the recovery module.
 */
Recov_Stats recov_Stats;

/*
 * For per-client statistics about recovery on the server.
 * This amounts to a per-host list, in array form.
 * Each host has numTries elements in the array.  The spriteID and numTries
 * fields are only initialized in the first element.
 */
typedef	struct	RecovPerHostInfo {
    int		spriteID;	/* Sprite ID of client. */
    Time	start;		/* First recovery attempt. */
    Time	finished;	/* First recovery attempt finished. */
    int		numTries;	/* Number of recovery attempts. */
    int		numHandles;	/* Number of reopens requested. */
    int		numSuccessful;	/* Handles successfully recovered. */
} RecovPerHostInfo;



/*
 * The state of other hosts is kept in a hash table keyed on SpriteID.
 * This state is maintained by Recov_HostAlive and Recov_HostDead, which are
 * called in turn after packet reception or RPC timeout, respectively.
 * Recov_HostDead is also called by the Rpc_Daemon if it can't get an
 * explicit acknowledgment from a client.
 */
static Hash_Table	recovHashTableStruct;
static Hash_Table	*recovHashTable = &recovHashTableStruct;

typedef	struct	RecovStampList {
    List_Links	timeStampList;
    Timer_Ticks	start;
    Timer_Ticks	finished;
    int		numHandles;		/* Handles since last time. */
    int		numSuccessful;		/* Successful last time. */
} RecovStampList;

typedef struct RecovHostState {
    int			state;		/* flags defined below */
    int			clientState;	/* flags defined in recov.h */
    int			spriteID;	/* Sprite Host ID */
    unsigned int	bootID;		/* Boot timestamp from RPC header */
    Time		time;		/* Time of last message */
    Sync_Condition	alive;		/* Notified when host comes up */
    Sync_Condition	recovery;	/* Notified when recovery is complete */
    List_Links		rebootList;	/* List of callbacks for when this
					 * host reboots. */
    int			numFailures;	/* Times a failure occurs during the
					 * reboot callbacks.  Such a failure
					 * triggers a retry of the reboot
					 * callbacks. */
    /*
     * The following fields are used in the tracing of the recovery module.
     */
    Timer_Ticks		start;		/* Time that recovery is started. */
    Timer_Ticks		finished;	/* Time recovery attempt  finishes. */
    int			numTries;	/* Number of times recov attempted. */
    int			numHandles;	/* Handles requested. */
    int			numSuccessful;	/* Successful handles. */
    int			currentHandles;	/* Temporary info. */
    int			currentSuccessful;
    List_Links		timeStampList;	/* List of time stamps for recovery. */
} RecovHostState;

#define RECOV_INIT_HOST(hostPtr, zspriteID, zstate, zbootID) \
    hostPtr = (RecovHostState *) malloc(sizeof (RecovHostState)); \
    (void)bzero((Address)hostPtr, sizeof(RecovHostState)); \
    List_Init(&(hostPtr)->rebootList); \
    List_Init(&(hostPtr)->timeStampList);\
    (hostPtr)->spriteID = zspriteID; \
    (hostPtr)->state = zstate; \
    (hostPtr)->bootID = zbootID; \
    (hostPtr)->numFailures = 0;

/*
 * Access to the hash table is monitored.
 */
static Sync_Lock recovLock;
#define LOCKPTR (&recovLock)


/*
 * recov_PrintLevel defines how noisey we are about other hosts.
 *	Values for the print level should be defined in increasing order.
 */
int recov_PrintLevel = RECOV_PRINT_REBOOT;

#define RecovHostPrint(level, spriteID, message) \
	if (recov_PrintLevel >= level) { \
	    Sys_HostPrint(spriteID, message); \
	}

Trace_Header recovTraceHdr;
Trace_Header *recovTraceHdrPtr = &recovTraceHdr;
int recovTraceLength = 50;
Boolean recovTracing = TRUE;

/*
 * Forward declarations.
 */

static void CrashCallBacks _ARGS_((ClientData data, Proc_CallInfo *callInfoPtr));
extern void DelayedCrashCallBacks _ARGS_((ClientData data, Proc_CallInfo *callInfoPtr));
static void CallBacksDone _ARGS_((int spriteID));
static void MarkRecoveryComplete _ARGS_((int spriteID));
static void MarkHostDead _ARGS_((int spriteID));
static void GetRebootList _ARGS_((List_Links *notifyListHdr, int spriteID));
static char *GetState _ARGS_((int state));
static void PrintExtraState _ARGS_((RecovHostState *hostPtr));




/*
 *----------------------------------------------------------------------
 *
 * Recov_Init --
 *
 *	Set up the data structures used by the recovery module.
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
Recov_Init()
{
    Sync_LockInitDynamic(&recovLock, "Recov:recovLock");
    Hash_Init(recovHashTable, 8, HASH_ONE_WORD_KEYS);
    List_Init(&crashCallBackList);
    Trace_Init(recovTraceHdrPtr, recovTraceLength,
		sizeof(RecovTraceRecord), 0);
    recov_CrashDelay = (unsigned int)(timer_IntOneMinute);
    RecovPingInit();
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * Recov_CrashRegister --
 *
 *	This procedure is used to register a crash callback procedure.
 *	This is typically done once at boot time by each module that
 *	is interested in learning about the failure of other hosts.
 *	When other hosts are (apparently) down the recovery module
 *	calls back to other modules that have registered via this procedure.
 *	This allows those other modules to clean up any state associated
 *	with the crashed host.
 *	
 * Results:
 *	None.
 *
 * Side effects:
 *	Callback entry added to the crash call-back list.
 *
 *----------------------------------------------------------------------
 */
void
Recov_CrashRegister(crashCallBackProc, crashData)
    void	(*crashCallBackProc)();
    ClientData	crashData;
{
    register	NotifyElement	*notifyPtr;

    notifyPtr = (NotifyElement *) malloc(sizeof (NotifyElement));
    notifyPtr->proc = crashCallBackProc;
    notifyPtr->data = crashData;
    List_InitElement((List_Links *) notifyPtr);
    List_Insert((List_Links *) notifyPtr, LIST_ATREAR(&crashCallBackList));
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * Recov_RebootRegister --
 *
 *	Schedule a callback for when a particular host reboots.
 *	To make sure we detect a crash, the recovery module has to
 *	periodically check on the state of the target host.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	This initiate a background callback to check-up on the host's state.
 *
 *----------------------------------------------------------------------
 */

ENTRY void
Recov_RebootRegister(spriteID, rebootCallBackProc, rebootData)
    int spriteID;
    void (*rebootCallBackProc)();
    ClientData rebootData;
{
    Hash_Entry *hashPtr;
    RecovHostState *hostPtr;
    register NotifyElement *notifyPtr;
    Boolean found = FALSE;

    LOCK_MONITOR;

    if (spriteID <= 0 || spriteID == rpc_SpriteID) {
	panic("Recov_RebootRegister, bad hostID %d\n", spriteID);
    } else {
	hashPtr = Hash_Find(recovHashTable, (Address)spriteID);
	if (hashPtr->value == (Address)NIL) {
	    RECOV_INIT_HOST(hostPtr, spriteID, RECOV_STATE_UNKNOWN, 0);
	    hashPtr->value = (Address)hostPtr;
	} else {
	    hostPtr = (RecovHostState *)hashPtr->value;
	}
	/*
	 * Save the callback while avoiding duplications.
	 */
	LIST_FORALL(&hostPtr->rebootList, (List_Links *)notifyPtr) {
	    if (notifyPtr->proc == rebootCallBackProc &&
		notifyPtr->data == rebootData) {
		found = TRUE;
		break;
	    }
	}
	if (!found) {
	    notifyPtr = (NotifyElement *) malloc(sizeof (NotifyElement));
	    notifyPtr->proc = rebootCallBackProc;
	    notifyPtr->data = rebootData;
	    notifyPtr->refCount = 1;
	    List_InitElement((List_Links *)notifyPtr);
	    List_Insert((List_Links *)notifyPtr,
			LIST_ATFRONT(&hostPtr->rebootList));
	} else {
	    notifyPtr->refCount++;
	}
	/*
	 * Mark the host as being interesting, and add it to the ping
	 * list if necessary.
	 */
	hostPtr->state |= RECOV_PINGING_HOST;
	RecovAddHostToPing(spriteID);
    }
    UNLOCK_MONITOR;
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * Recov_RebootUnRegister --
 *
 *	Remove a callback for when a particular host reboots.  This is
 *	used after we are no longer interested in a host rebooting.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Nukes the reboot procedure.  If all interested parties remove their
 *	reboot callbacks then the periodic check of the other host is
 *	stopped.
 *
 *----------------------------------------------------------------------
 */

ENTRY void
Recov_RebootUnRegister(spriteID, rebootCallBackProc, rebootData)
    int spriteID;
    void (*rebootCallBackProc)();
    ClientData rebootData;
{
    Hash_Entry *hashPtr;
    RecovHostState *hostPtr;
    register NotifyElement *notifyPtr;
    Boolean found = FALSE;

    LOCK_MONITOR;

    if (spriteID <= 0 || spriteID == rpc_SpriteID) {
	panic("Recov_RebootUnRegister, bad hostID %d\n", spriteID);
    } else {
	hashPtr = Hash_Find(recovHashTable, (Address)spriteID);
	if (hashPtr->value == (Address)NIL) {
	    RECOV_INIT_HOST(hostPtr, spriteID, RECOV_STATE_UNKNOWN, 0);
	    hashPtr->value = (Address)hostPtr;
	} else {
	    hostPtr = (RecovHostState *)hashPtr->value;
	}
	/*
	 * Look for the matching callback.
	 */
	LIST_FORALL(&hostPtr->rebootList, (List_Links *)notifyPtr) {
	    if (notifyPtr->proc == rebootCallBackProc &&
		notifyPtr->data == rebootData) {
		found = TRUE;
		break;
	    }
	}
	if (found) {
	    notifyPtr->refCount--;
	    if (notifyPtr->refCount <= 0) {
		int		num;
		/*
		 * Mousetrap for debugging recovery reference count problem.
		 */
		if (notifyPtr->proc == (void((*)())) Fsutil_Reopen) {

		    if (recov_PrintLevel >= RECOV_PRINT_CRASH) {
			printf(
		"Recov: deleting Fsutil_Reopen for server %d ref count %d\n",
			    spriteID, notifyPtr->refCount);
		    }
		    /*
		     * We want to panic if we still have handles for
		     * this server.
		     */
		    num = Fsutil_TestForHandles(spriteID);
		    /*
		     * This routine is called before the handle is removed,
		     * so we must take into account the fact that it still
		     * exists in the handle table.
		     */
		    if (num > 1) {
			printf("%d file and device handles remain\n", num);
			panic("Shouldn't have deleted it - handles remain!\n");
		    }
		}
		List_Remove((List_Links *)notifyPtr);
		free((Address)notifyPtr);
	    }
	}
    }
    UNLOCK_MONITOR;
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * Recov_HostAlive --
 *
 *	Mark the host as being alive.  This is called when we've received
 *	a message from the host.  It uses state from the host table and
 *	the bootID parameter to detect reboots.  If a reboot is detected,
 *	but we thought the host was up, then the Crash call-backs are invoked.
 *	In any case, a reboot invokes the Reboot call-backs, if any.
 *
 *	This procedure is called from client RPC upon successful completion
 *	of an RPC, and by server RPC upon reciept of a client request.
 *	These two cases are identified by the 'asyncRecovery' parameter.
 *	Servers want synchronous recovery so they don't service anything
 *	until state associated with that client has been cleaned up via
 *	the Crash call-backs.  So Recov_HostAlive blocks (if !asyncRecovery)
 *	until the crash call-backs are complete.  Clients don't have the
 *	same worries so they let the crash call-backs complete in the
 *	background (asyncRecovery is TRUE).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Updates the boot timestamp of the other host.  Procedures installed
 *	with Recov_CrashRegister are called when the bootID changes.  A
 *	timestamp of when this message was received is obtained from the
 *	"cheap" clock so we can tell later if there has been recent message
 *	traffic.
 *
 *----------------------------------------------------------------------
 */

ENTRY void
Recov_HostAlive(spriteID, bootID, asyncRecovery, rpcNotActive)
    int spriteID;		/* Host ID of the message sender */
    unsigned int bootID;	/* Boot time stamp from message header */
    Boolean asyncRecovery;	/* TRUE means do recovery call-backs in
				 * the background. FALSE causes the process
				 * to wait until crash recovery is complete. */
    Boolean rpcNotActive;	/* This is a flag propogated from the rpc
				 * packet header.  If set it means the RPC
				 * system on the remote host isn't fully
				 * turned on.  Reboot recovery is delayed
				 * until this changes. */
{
    register Hash_Entry *hashPtr;
    register RecovHostState *hostPtr;

    LOCK_MONITOR;
    if (spriteID == NET_BROADCAST_HOSTID || bootID == 0 || sys_ShuttingDown) {
	/*
	 * Don't track the broadcast address.  Also ignore zero valued
	 * bootIDs.  These come from hosts at early boot time, or
	 * in certain error conditions like trying to send too much
	 * data in a single RPC.  Also don't bother to check things
	 * where we are shutting down the system because we don't want 
	 * RPCs for the cache data to get blocked.
	 */
	UNLOCK_MONITOR;
	return;
    }

    recov_Stats.packets++;
    hashPtr = Hash_Find(recovHashTable, (Address)spriteID);
    if (hashPtr->value == (Address)NIL) {
	/*
	 * Initialize the host's state. This is the first time we've talked
	 * to it since we've been up, so take no action.
	 */
	RECOV_INIT_HOST(hostPtr, spriteID, RECOV_HOST_ALIVE, bootID);
	hashPtr->value = (Address)hostPtr;

	RecovHostPrint(RECOV_PRINT_IF_UP, spriteID, "is up\n");
	RECOV_TRACE(spriteID, RECOV_HOST_ALIVE, RECOV_CUZ_INIT);
    } else {
	hostPtr = (RecovHostState *)hashPtr->value;
    }
    /*
     * Have to read the clock in order to suppress repeated pings,
     * see Recov_GetHostState and Recov_IsHostDown.
     */
    Timer_GetTimeOfDay(&hostPtr->time, (int *)NIL, (Boolean *)NIL);
    /*
     * Check for a rebooted peer by comparing boot time stamps.
     */
    if (hostPtr->bootID != bootID) {
	if (hostPtr->bootID != 0) {
	    RecovHostPrint(RECOV_PRINT_REBOOT, spriteID, "rebooted\n");
	} else {
	    /*
	     * We initialized state before talking to the host the first time.
	     * The state is 'unknown' so we won't do crash call-backs.
	     */
	}
	hostPtr->bootID = bootID;
	RECOV_TRACE(spriteID, hostPtr->state, RECOV_CUZ_REBOOT);
	if (hostPtr->state &
		(RECOV_HOST_ALIVE|RECOV_HOST_DYING|RECOV_HOST_BOOTING)) {
		RecovHostPrint(RECOV_PRINT_ALL, spriteID,
			"Undetected crash occurred.\n");
	    /*
	     * A crash occured un-detected.  We do the crash call-backs
	     * first, and block server processes in the meantime.
	     * RECOV_CRASH_CALLBACKS flag is cleared by CrashCallBacks.
	     */
	    hostPtr->state &=
		    ~(RECOV_HOST_ALIVE|RECOV_HOST_DYING|RECOV_HOST_DEAD);
	    hostPtr->state |= RECOV_HOST_BOOTING;
	    RECOV_TRACE(spriteID, hostPtr->state, RECOV_CUZ_CRASH_UNDETECTED);
	    if ((hostPtr->state & RECOV_CRASH_CALLBACKS) == 0) {
		hostPtr->state |= RECOV_CRASH_CALLBACKS;
		RECOV_TRACE(spriteID, hostPtr->state, RECOV_CUZ_CRASH_UNDETECTED);
		Proc_CallFunc(CrashCallBacks, (ClientData)spriteID, 0);
	    }
	}
    } else  if ( ! (hostPtr->state &
	    (RECOV_CRASH_CALLBACKS|RECOV_WANT_RECOVERY)) &&
	    (hostPtr->state & RECOV_HOST_ALIVE)) {
	/*
	 * Fast path.  We already think the other host is up, it didn't
	 * reboot, we don't want recovery, and there are no pending
	 * crash call-backs to synchronize with.
	 */
	goto exit;
    }
    /*
     * Block servers until crash recovery actions complete.
     * This prevents servicing requests from clients until after the
     * recovery actions complete.
     */
    if (! asyncRecovery) {
	RecovHostPrint(RECOV_PRINT_ALL, spriteID, "Async recovery false.\n");
	while (hostPtr->state & RECOV_CRASH_CALLBACKS) {
	    (void)Sync_Wait(&hostPtr->recovery, FALSE);
	    if (sys_ShuttingDown) {
		UNLOCK_MONITOR;
		Proc_Exit(1);
	    }
	}
    }
    /*
     * Now that we've taken care of crash recovery, we see if the host
     * is newly up.  If so, invoke any reboot call-backs and notify
     * waiting processes. This means clientA (us) may start
     * re-opening files from serverB (the other guy) at the same time
     * as clientA (us) is closing files that serverB had had open.
     * ie. both the crash and reboot call backs may proceed in parallel.
     */
    switch(hostPtr->state &
       (RECOV_HOST_ALIVE|RECOV_HOST_BOOTING|RECOV_HOST_DEAD|RECOV_HOST_DYING)) {
        case RECOV_STATE_UNKNOWN:	/* This is zero, no bits set */
	    /*
	     * We have uninitialized state for the host, mark it alive.
	     */
	    RecovHostPrint(RECOV_PRINT_IF_UP, spriteID, "is up\n");
	    if (rpcNotActive) {
		hostPtr->state |= RECOV_HOST_BOOTING;
	    } else {
		hostPtr->state |= RECOV_HOST_ALIVE;
	    }
	    break;
	case RECOV_HOST_ALIVE:
	    /*
	     * Host already alive.  We may still want recovery at this
	     * point.  See CallBacksDone.
	     */
	    RecovHostPrint(RECOV_PRINT_ALL, spriteID, "Already up.\n");
	    break;
	case RECOV_HOST_BOOTING:
	    /*
	     * See if a booting host is ready yet.
	     */
	    RecovHostPrint(RECOV_PRINT_ALL, spriteID, "Booting, set recov.\n");
	    if (! rpcNotActive) {
		hostPtr->state &= ~RECOV_HOST_BOOTING;
		hostPtr->state |= RECOV_HOST_ALIVE|RECOV_WANT_RECOVERY;
		RecovHostPrint(RECOV_PRINT_ALL, spriteID,
			"Booting, set alive, recov.\n");
	    }
	    break;
	case RECOV_HOST_DYING:
	case RECOV_HOST_DEAD:
	    /*
	     * See if the host is newly booting or back from a net partition.
	     */
	    if (rpcNotActive) {
		hostPtr->state |= RECOV_HOST_BOOTING;
		RecovHostPrint(RECOV_PRINT_ALL, spriteID,
			"Dead or dying, set booting.\n");
	    } else {
		hostPtr->state |= (RECOV_HOST_ALIVE|RECOV_WANT_RECOVERY);
		RecovHostPrint(RECOV_PRINT_ALL, spriteID,
			"Dead, dying, set want recov.\n");
	    }
	    hostPtr->state &= ~(RECOV_HOST_DEAD|RECOV_HOST_DYING);
	    break;
	default:
	    printf("Unexpected recovery state <%x> for ", hostPtr->state);
	    Sys_HostPrint(spriteID, "\n");
	    break;
    }
    /*
     * After a host comes up enough to support RPC service, we
     * initiate reboot recovery if needed.
     */
    if ((hostPtr->state & RECOV_WANT_RECOVERY) &&
	(hostPtr->state & RECOV_HOST_ALIVE) &&
	(hostPtr->state & RECOV_REBOOT_CALLBACKS) == 0) {
	hostPtr->state &= ~RECOV_WANT_RECOVERY;
	hostPtr->state |= RECOV_REBOOT_CALLBACKS;
	RecovHostPrint(RECOV_PRINT_ALL, spriteID,
		"Want recov, etc, callbacks.\n");
	Proc_CallFunc(RecovRebootCallBacks, (ClientData)spriteID, 0);
    }
exit:
    UNLOCK_MONITOR;
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * Recov_HostDead --
 *
 *	Change the host's state to "dead".  This is called from client RPC
 *	when an RPC timed out with no response.  It is also called by the
 *	Rpc_Daemon when it can't recontact a client to get an explicit
 *	acknowledgment.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If the host was previously thought up, this sets the state in
 *	the host state table to dead and invokes the crash callbacks.
 *
 *----------------------------------------------------------------------
 */

ENTRY void
Recov_HostDead(spriteID)
    int spriteID;
{
    register Hash_Entry *hashPtr;
    register RecovHostState *hostPtr;

    LOCK_MONITOR;
    if (spriteID == NET_BROADCAST_HOSTID || rpc_NoTimeouts) {
	/*
	 * If rpcNoTimeouts is set the Rpc_Daemon may still call us if
	 * it can't get an acknowledgment from a host to close down
	 * a connection.  We ignore this so that we don't take action
	 * against the offending host (who is probably in the debugger)
	 * (Hmm, it doesn't look like Rpc_Daemon calls this procedure.)
	 */
	UNLOCK_MONITOR;
	return;
    }

    recov_Stats.timeouts++;
    hashPtr = Hash_Find(recovHashTable, (Address)spriteID);
    if (hashPtr->value == (Address)NIL) {
	RECOV_INIT_HOST(hostPtr, spriteID, RECOV_HOST_DEAD, 0);
	hashPtr->value = (Address)hostPtr;
    } else {
	hostPtr = (RecovHostState *)hashPtr->value;
    }
    switch(hostPtr->state &
	    (RECOV_HOST_ALIVE|RECOV_HOST_BOOTING|RECOV_HOST_DEAD)) {
	case RECOV_HOST_DEAD:
	case RECOV_HOST_DYING:
	    /*
	     * Host already dead or dying.
	     */
	    break;
	case RECOV_STATE_UNKNOWN:
	case RECOV_HOST_BOOTING:
	case RECOV_HOST_ALIVE:
	    hostPtr->state &=
		~(RECOV_HOST_ALIVE|RECOV_HOST_BOOTING);
	    /*
	     * Special handling if we abort during the recovery protocol.
	     * In this case it is possible for the other host to go from
	     * alive to dead and back to alive before the recovery protocol
	     * finally terminates.  If that happens we could loose a reboot
	     * event and fail to initiate recovery again.  We mark the
	     * host specially so the reboot callbacks are retried.
	     */
	    if (hostPtr->state & RECOV_REBOOT_CALLBACKS) {
		hostPtr->state |= RECOV_FAILURE;
	    }
	    /*
	     * After an RPC timeout (which is already logged by RPC to syslog)
	     * make the crash call backs.  These are made after a delay
	     * if dying_state is defined.  This helps smooth over temporary
	     * communication failures.
	     *
	     */
#ifdef dying_state
	    hostPtr->state |= RECOV_HOST_DYING;
	    Proc_CallFunc(DelayedCrashCallBacks, (ClientData)spriteID,
			    recov_CrashDelay);
#else
	    hostPtr->state |= RECOV_HOST_DEAD|RECOV_CRASH_CALLBACKS;
	    RecovHostPrint(RECOV_PRINT_CRASH, spriteID,
		    "crash call-backs made\n");
	    RECOV_TRACE(spriteID, hostPtr->state, RECOV_CUZ_CRASH);
	    Proc_CallFunc(CrashCallBacks, (ClientData)spriteID, 0);
#endif
	    break;
    }
    UNLOCK_MONITOR;
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * Recov_IsHostDown --
 *
 *	This decides if the specified host is down.  If the host is known
 *	to be down this routine	returns FAILURE.  SUCCESS is returned if
 *	the host is alive, and RPC_SERVICE_DISABLED is returned if the
 *	host is in its boot sequence and can't service RPC's yet.  If there
 *	hasn't been recent (within the last 10 seconds) message traffic
 *	this this pings the host to find out for sure its state.
 *
 * Results:
 *	SUCCESS if the host is up, FAILURE if it doesn't respond to
 *	pings or is known to be down, and RPC_SERVICE_DISABLED if
 *	the host says so.
 *
 * Side effects:
 *	May do a ping.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Recov_IsHostDown(spriteID)
    int spriteID;
{
    register ReturnStatus status = SUCCESS;

    if (spriteID == NET_BROADCAST_HOSTID) {
	printf("Warning: Recov_IsHostDown, got broadcast address\n");
	return(SUCCESS);
    }
    switch (Recov_GetHostState(spriteID)) {
	case RECOV_STATE_UNKNOWN:
	    RECOV_TRACE(spriteID, RECOV_STATE_UNKNOWN, RECOV_CUZ_PING_ASK);
	    recov_Stats.pings++;
	    status = Rpc_Ping(spriteID);
	    break;
	case RECOV_HOST_BOOTING:
	case RECOV_HOST_ALIVE:
	case RECOV_HOST_DYING:	/* fake it to allow for the grace period */
	    recov_Stats.pingsSuppressed++;
	    status = SUCCESS;
	    break;
	case RECOV_HOST_DEAD:
	    status = FAILURE;
	    break;
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Recov_HostTrace --
 *
 *	Add an entry to the recovery trace.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ENTRY void
Recov_HostTrace(spriteID, event)
    int spriteID;
    int event;
{
    /*
     * No monitor lock needed here, since Trace_Insert does its own
     * synchronization.
     */
    RECOV_TRACE(spriteID, RECOV_STATE_UNKNOWN, event);
}

/*
 *----------------------------------------------------------------------
 *
 * Recov_GetClientState --
 *
 *	Return the client state associated with a host.  The recovery host
 *	table is a convenient object keyed on spriteID.  Other modules can
 *	set their own state in the table (beyond the simple up/down state
 *	mainted by the rest of this module), and retrieve it with this call.
 *
 * Results:
 *	A copy of the clientState field.  0 is returned if there is no
 *	host table entry.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ENTRY int
Recov_GetClientState(spriteID)
    int spriteID;
{
    Hash_Entry *hashPtr;
    RecovHostState *hostPtr;
    int stateBits = 0;

    LOCK_MONITOR;

    hashPtr = Hash_LookOnly(recovHashTable, (Address)spriteID);
    if (hashPtr != (Hash_Entry *)NIL) {
	hostPtr = (RecovHostState *)hashPtr->value;
	if (hostPtr != (RecovHostState *)NIL) {
	    stateBits = hostPtr->clientState;
	}
    }
    UNLOCK_MONITOR;
    return(stateBits);
}

/*
 *----------------------------------------------------------------------
 *
 * Recov_SetClientState --
 *
 *	Set a client state bit.  This or's the parameter into the
 *	client state word.  The previous value of the client state
 *	word is returned so this procedure can be used like test-and-set.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets bits in the clientState field of the host state.  This will add
 *	an entry to the host table if one doesn't alreay exist.  Its RPC
 *	up/down state is set to "unknown" in this case.
 *
 *----------------------------------------------------------------------
 */

ENTRY int
Recov_SetClientState(spriteID, stateBits)
    int spriteID;
    int stateBits;
{
    Hash_Entry *hashPtr;
    RecovHostState *hostPtr;
    register oldState;
    RecovStampList	*stampPtr;

    LOCK_MONITOR;

    hashPtr = Hash_Find(recovHashTable, (Address)spriteID);
    hostPtr = (RecovHostState *)hashPtr->value;
    if (hostPtr == (RecovHostState *)NIL) {
	RECOV_INIT_HOST(hostPtr, spriteID, RECOV_STATE_UNKNOWN, 0);
	hashPtr->value = (Address)hostPtr;
    }
    if ((stateBits & CLT_RECOV_IN_PROGRESS) != 0) {
	if (hostPtr->numTries == 0) {
	    /* First recovery attempt */
	    if ((hostPtr->clientState & CLT_RECOV_IN_PROGRESS) != 0) {
		printf("No recovery attempt yet, but marked as in progress.");
	    }
	    Timer_GetCurrentTicks(&hostPtr->start);
	} else {
	    /* Add a time-stamp to the recovery list. */
	    stampPtr = (RecovStampList *) malloc(sizeof (RecovStampList));
	    Timer_GetCurrentTicks(&stampPtr->start);
	    List_InitElement((List_Links *) stampPtr);
	    List_Insert((List_Links *) stampPtr,
		    LIST_ATREAR(&hostPtr->timeStampList));
	    /*
	     * Clear handle count for this round.
	     */
	    hostPtr->currentHandles = 0;
	    hostPtr->currentSuccessful = 0;
	}
	hostPtr->numTries++;
    }

    oldState = hostPtr->clientState;
    hostPtr->clientState |= stateBits;
    UNLOCK_MONITOR;
    return(oldState);
}

/*
 *----------------------------------------------------------------------
 *
 * Recov_ClearClientState --
 *
 *	Clear client state bits.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Clears bits in the clientState field of the host state.  This does
 *	nothing if the state doesn't exist.
 *
 *----------------------------------------------------------------------
 */

ENTRY void
Recov_ClearClientState(spriteID, stateBits)
    int spriteID;
    int stateBits;
{
    register Hash_Entry		*hashPtr;
    register RecovHostState	*hostPtr = (RecovHostState *) NIL;
    RecovStampList		*stampPtr;

    LOCK_MONITOR;

    hashPtr = Hash_LookOnly(recovHashTable, (Address)spriteID);
    if (hashPtr != (Hash_Entry *)NIL) {
	hostPtr = (RecovHostState *)hashPtr->value;
	if (hostPtr != (RecovHostState *)NIL) {
	    hostPtr->clientState &= ~stateBits;
	}
    }
    /* End of recovery? */
    if ((hostPtr != (RecovHostState *) NIL) &&
	    (stateBits & CLT_RECOV_IN_PROGRESS) != 0) {
	/* End of 1st recovery try? */
	if (hostPtr->numTries <= 1) {
	    Timer_GetCurrentTicks(&hostPtr->finished);
	    /* Final count of handles recovered is in hostPtr. */
	    hostPtr->numHandles = hostPtr->currentHandles;
	    hostPtr->numSuccessful = hostPtr->currentSuccessful;
	} else {
	    if (List_IsEmpty(&hostPtr->timeStampList)) {
		printf("Recov_ClearClientState: timeStampList is empty!\n");
		hostPtr->numSuccessful = 0;	/* signal the error */
	    } else {
		stampPtr = (RecovStampList *)
			List_Last((List_Links *) &hostPtr->timeStampList);
		Timer_GetCurrentTicks(&stampPtr->finished);
		stampPtr->numHandles = hostPtr->currentHandles;
		stampPtr->numSuccessful = hostPtr->currentSuccessful;
	    }
	}
    }
    UNLOCK_MONITOR;
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * Recov_AddHandleCountToClientState --
 *
 *	Increment count of handles reopened from this client.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Data in per-host recovery info updated.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
Recov_AddHandleCountToClientState(type, clientID, status)
    int			type;		/* Type of handle being reopened. */
    int			clientID;	/* Id of client requesting reopen. */
    ReturnStatus	status;		/* Whether the reopen succeeded. */
{
    register Hash_Entry *hashPtr;
    register RecovHostState *hostPtr = (RecovHostState *) NIL;

    LOCK_MONITOR;

    hashPtr = Hash_LookOnly(recovHashTable, (Address)clientID);
    if (hashPtr != (Hash_Entry *)NIL) {
	hostPtr = (RecovHostState *)hashPtr->value;
	if (hostPtr != (RecovHostState *)NIL) {
	    hostPtr->currentHandles++;
	    if (status == SUCCESS) {
		hostPtr->currentSuccessful++;
	    }
	}
    }
    UNLOCK_MONITOR;
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * Recov_DumpClientRecovInfo --
 *
 *	Dump out some of the recovery statistics in the per-host info.
 *
 * Results:
 *	Returns FAILURE if recovery still in progress.  Returns SUCCESS
 *	otherwise.
 *
 * Side effects:
 *	Info copied into buffer.  Size of needed buffer also copied out.
 *
 *----------------------------------------------------------------------
 */
ENTRY ReturnStatus
Recov_DumpClientRecovInfo(length, resultPtr, lengthNeededPtr)
    int			length;			/* size of data buffer */
    Address		resultPtr;		/* Array of info structs. */
    int			*lengthNeededPtr;	/* to return space needed */
{
    Hash_Entry		*hashPtr;
    RecovHostState	*hostPtr;
    Hash_Search		hashSearch;
    RecovPerHostInfo	*infoPtr;
    int			numNeeded;
    int			numAvail;

    LOCK_MONITOR;

    /*
     * If recovery still going on, return FAILURE.
     * NOTE: This isn't a sure-fire test.  I'm not sure there is one right now.
     */
    if (fsutil_NumRecovering >= 1) {
	UNLOCK_MONITOR;
	return FAILURE;
    }
    if (resultPtr != (Address) NIL) {
	bzero(resultPtr, length);
    }
    numNeeded = 0;
    numAvail = length / sizeof (RecovPerHostInfo);

    infoPtr = (RecovPerHostInfo *) resultPtr;
    Hash_StartSearch(&hashSearch);
    for (hashPtr = Hash_Next(recovHashTable, &hashSearch);
	    hashPtr != (Hash_Entry *) NIL;
	    hashPtr = Hash_Next(recovHashTable, &hashSearch)) {
	hostPtr = (RecovHostState *)hashPtr->value;

	/*
	 * We need one slot for each host, whether numTries is 0 or 1, plus
	 * additional slots for each numTries over 1.
	 */
	numNeeded++;
	if (hostPtr->numTries > 1) {
	    numNeeded += (hostPtr->numTries - 1);
	}
	if (numNeeded > numAvail) {
	    continue;
	}
	/* Why didn't Brent use GetValue()??? */
	if (hostPtr != (RecovHostState *) NIL) {
	    RecovStampList	*stampPtr;

	    /* Copy info into buffer */
	    infoPtr->spriteID = hostPtr->spriteID;
	    infoPtr->numTries = hostPtr->numTries;
	    Timer_GetRealTimeFromTicks(hostPtr->start,
		    &(infoPtr->start), (int *) NIL, (Boolean *) NIL);
	    Timer_GetRealTimeFromTicks(hostPtr->finished,
		    &(infoPtr->finished), (int *)NIL, (Boolean *) NIL);
	    infoPtr->numHandles = hostPtr->numHandles;
	    infoPtr->numSuccessful = hostPtr->numSuccessful;
	    LIST_FORALL(&hostPtr->timeStampList, (List_Links *) stampPtr) {
		infoPtr++;
		Timer_GetRealTimeFromTicks(stampPtr->start,
			&infoPtr->start, (int *) NIL, (Boolean *) NIL);
		Timer_GetRealTimeFromTicks(stampPtr->finished,
			&infoPtr->finished, (int *) NIL, (Boolean *) NIL);
		infoPtr->numHandles = stampPtr->numHandles;
		infoPtr->numSuccessful = stampPtr->numSuccessful;
	    }
	}
	infoPtr++;
    }
    *lengthNeededPtr = numNeeded * sizeof (RecovPerHostInfo);
    UNLOCK_MONITOR;

    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * RecovRebootCallBacks --
 *
 *	This calls the call-back procedures installed by other modules
 *	via Recov_RebootRegister.  It is invoked asynchronously from
 *	Recov_HostAlive when that procedure detects a reboot.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Invoke the call-backs.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
void
RecovRebootCallBacks(data, callInfoPtr)
    ClientData data;
    Proc_CallInfo *callInfoPtr;
{
    List_Links notifyList;
    register NotifyElement *notifyPtr;
    register int spriteID = (int)data;

    GetRebootList(&notifyList, spriteID);
    recov_Stats.reboots++;
    while (!List_IsEmpty(&notifyList)) {
	notifyPtr = (NotifyElement *)List_First(&notifyList);
	(*notifyPtr->proc)(spriteID, notifyPtr->data);
	List_Remove((List_Links *)notifyPtr);
	free((Address)notifyPtr);
    }
    CallBacksDone(spriteID);
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * GetRebootList --
 *
 *	Copy out the list of reboot callbacks.  The list is protected by
 * 	a monitor, but we don't want to call any recovery procedures from
 *	inside that monitor so we make a copy.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Copy the reboot list off the host state table and return it
 *	to our caller who should free up the copied elements.
 *
 *----------------------------------------------------------------------
 */

ENTRY static void
GetRebootList(notifyListHdr, spriteID)
    List_Links *notifyListHdr;
    int spriteID;
{
    register Hash_Entry *hashPtr;
    register RecovHostState *hostPtr;
    register NotifyElement *notifyPtr;
    register NotifyElement *newNotifyPtr;

    LOCK_MONITOR;

    hashPtr = Hash_LookOnly(recovHashTable, (Address)spriteID);
    hostPtr = (RecovHostState *)hashPtr->value;
    List_Init(notifyListHdr);
    LIST_FORALL(&hostPtr->rebootList, (List_Links *)notifyPtr) {
	newNotifyPtr = (NotifyElement *) malloc(sizeof (NotifyElement));
	newNotifyPtr->proc = notifyPtr->proc;
	newNotifyPtr->data = notifyPtr->data;
	List_InitElement((List_Links *)newNotifyPtr);
	List_Insert((List_Links *)newNotifyPtr, LIST_ATREAR(notifyListHdr));
    }
    UNLOCK_MONITOR;
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * CallBacksDone --
 *
 *	Clear the internal state bit that says callbacks are in progress.
 *	This checks to see if there was a communication failure during
 *	the reboot callbacks.  If so, the WANT_RECOVERY bit is set
 *	to ensure that another set of reboot callbacks are made.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Clears RECOV_REBOOT_CALLBACKS and RECOV_FAILURE.  May set
 *	RECOV_WANT_RECOVERY if RECOV_FAILURE was set.
 *
 *----------------------------------------------------------------------
 */

ENTRY static void
CallBacksDone(spriteID)
    int spriteID;
{
    register Hash_Entry *hashPtr;
    register RecovHostState *hostPtr;

    LOCK_MONITOR;

    hashPtr = Hash_LookOnly(recovHashTable, (Address)spriteID);
    hostPtr = (RecovHostState *)hashPtr->value;
    hostPtr->state &= ~RECOV_REBOOT_CALLBACKS;
    if (hostPtr->state & (RECOV_FAILURE)) {
	/*
	 * There has been a communication failure during the reboot callbacks.
	 */
	hostPtr->numFailures++;
	hostPtr->state &= ~RECOV_FAILURE;
	hostPtr->state |= RECOV_WANT_RECOVERY;
    } else {
	hostPtr->numFailures = 0;
    }
    UNLOCK_MONITOR;
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * CrashCallBacks --
 *
 *	Invoked asynchronously so that other modules
 *	can clean up behind the crashed host.  When done the host
 *	is marked as having recovery complete.  This unblocks server
 *	processes stalled in Recov_HostAlive.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Invoke the crash call-backs.
 *	Clears the recovery in progress flag checked in Recov_HostAlive.
 *
 *----------------------------------------------------------------------
 */

static void
CrashCallBacks(data, callInfoPtr)
    ClientData data;
    Proc_CallInfo *callInfoPtr;
{
    register NotifyElement *notifyPtr;
    register int spriteID = (int)data;

    recov_Stats.crashes++;
    LIST_FORALL(&crashCallBackList, (List_Links *)notifyPtr) {
	if (notifyPtr->proc != (void (*)())NIL) {
	    (*notifyPtr->proc)(spriteID, notifyPtr->data);
	 }
    }
    MarkRecoveryComplete(spriteID);
    RECOV_TRACE(spriteID, RECOV_CRASH, RECOV_CUZ_DONE);
    callInfoPtr->interval = 0;	/* Don't call again */
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * DelayedCrashCallBacks --
 *
 *	Invoked asynchronously from Recov_HostDead.  This is called after
 *	a grace period defined by recov_CrashDelay so that, for example,
 *	clients can be debugged without having the server close all
 *	their files.  When a client reboots, hoever, the crash callbacks
 *	will be sure to be called so other modules can clean up.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Invoke the crash call-backs.
 *	Clears the recovery in progress flag checked in Recov_HostAlive.
 *
 *----------------------------------------------------------------------
 */

static void
DelayedCrashCallBacks(data, callInfoPtr)
    ClientData data;
    Proc_CallInfo *callInfoPtr;
{
    register NotifyElement *notifyPtr;
    register int spriteID = (int)data;
    int state;

    state = Recov_GetHostState(spriteID);
    if (state & RECOV_HOST_DYING) {
	RecovHostPrint(RECOV_PRINT_CRASH, spriteID,
	    "crash call-backs being made\n");
	recov_Stats.crashes++;
	MarkHostDead(spriteID);
	LIST_FORALL(&crashCallBackList, (List_Links *)notifyPtr) {
	    if (notifyPtr->proc != (void (*)())NIL) {
		(*notifyPtr->proc)(spriteID, notifyPtr->data);
	     }
	}
	MarkRecoveryComplete(spriteID);
    } else if ((state & RECOV_HOST_DEAD) == 0) {
	recov_Stats.nonCrashes++;
    }
    callInfoPtr->interval = 0;	/* Don't call again */
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * MarkRecoveryComplete --
 *
 *	The recovery call-backs have completed, and this procedure's
 *	job is to mark that fact in the host hash table and to notify
 *	any processes that are blocked in Recov_HostAlive waiting for this.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets the state, if any, in the host state table.
 *	Notifies the hostPtr->recovery condition
 *
 *----------------------------------------------------------------------
 */

ENTRY static void
MarkRecoveryComplete(spriteID)
    int	spriteID;
{
    register Hash_Entry *hashPtr;
    register RecovHostState *hostPtr;

    LOCK_MONITOR;

    hashPtr = Hash_LookOnly(recovHashTable, (Address)spriteID);
    if (hashPtr != (Hash_Entry *)NIL) {
	hostPtr = (RecovHostState *)hashPtr->value;
	if (hostPtr != (RecovHostState *)NIL) {
	    hostPtr->state &= ~RECOV_CRASH_CALLBACKS;
	    Sync_Broadcast(&hostPtr->recovery);
	}
    }
    UNLOCK_MONITOR;
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * MarkHostDead --
 *
 *	Monitored procedure to change a host's state from dying to dead.
 *	This is done after the grace period has expired and we are
 *	about to call the crash callbacks.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Set the state to RECOV_HOST_DEAD
 *
 *----------------------------------------------------------------------
 */

ENTRY static void
MarkHostDead(spriteID)
    int	spriteID;
{
    register Hash_Entry *hashPtr;
    register RecovHostState *hostPtr;

    LOCK_MONITOR;

    hashPtr = Hash_LookOnly(recovHashTable, (Address)spriteID);
    if (hashPtr != (Hash_Entry *)NIL) {
	hostPtr = (RecovHostState *)hashPtr->value;
	if (hostPtr != (RecovHostState *)NIL) {
	    hostPtr->state &= ~RECOV_HOST_DYING;
	    hostPtr->state |= RECOV_HOST_DEAD;
	}
    }
    UNLOCK_MONITOR;
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * Recov_GetHostState --
 *
 *	This looks into	the host table to see and provides a guess
 *	as to the host's current state.  It uses a timestamp kept in
 *	the host state to see if there's been recent message traffic.
 *	If so, RECOV_HOST_ALIVE is returned.  If not, RECOV_STATE_UNKNOWN
 *	is returned and the caller should ping to make sure.  Finally,
 *	if it is known that the host is down already, then RECOV_HOST_DEAD
 *	is returned.
 *
 * Results:
 *	RECOV_STATE_UNKNOWN if the caller should ping to make sure.
 *	RECOV_HOST_ALIVE if the host is up (recent message traffic).
 *	RECOV_HOST_DEAD if the host is down (recent timeouts).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ENTRY int
Recov_GetHostState(spriteID)
    int spriteID;
{
    register Hash_Entry *hashPtr;
    register RecovHostState *hostPtr;
    register int state = RECOV_STATE_UNKNOWN;
    Time time;

    LOCK_MONITOR;

    hashPtr = Hash_LookOnly(recovHashTable, (Address)spriteID);
    if (hashPtr != (Hash_Entry *)NIL) {
	hostPtr = (RecovHostState *)hashPtr->value;
	if (hostPtr != (RecovHostState *)NIL) {
	    state = hostPtr->state &
	 (RECOV_HOST_ALIVE|RECOV_HOST_BOOTING|RECOV_HOST_DYING|RECOV_HOST_DEAD);
	    if (state & (RECOV_HOST_ALIVE|RECOV_HOST_BOOTING)) {
		/*
		 * Check for recent message traffic before admitting
		 * that the other machine is up.
		 */
		Timer_GetTimeOfDay(&time, (int *)NIL, (Boolean *)NIL);
		Time_Subtract(time, hostPtr->time, &time);
		if (Time_GT(time, time_TenSeconds)) {
		    state = RECOV_STATE_UNKNOWN;
		}
	    }
	}
    }
    UNLOCK_MONITOR;
    return(state);
}

/*
 *----------------------------------------------------------------------
 *
 * RecovGetLastHostState --
 *
 *	This looks into	the host table to pass back the
 *	host's current state.  It just uses whatever state the
 *	host has marked currently, and does no further interpretation.
 *
 * Results:
 *	hostPtr->state
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ENTRY int
RecovGetLastHostState(spriteID)
    int spriteID;
{
    register Hash_Entry *hashPtr;
    register RecovHostState *hostPtr;
    register int state = RECOV_STATE_UNKNOWN;

    LOCK_MONITOR;

    hashPtr = Hash_LookOnly(recovHashTable, (Address)spriteID);
    if (hashPtr != (Hash_Entry *)NIL) {
	hostPtr = (RecovHostState *)hashPtr->value;
	if (hostPtr != (RecovHostState *)NIL) {
	    state = hostPtr->state;
	}
    }
    UNLOCK_MONITOR;
    return(state);
}

/*
 *----------------------------------------------------------------------
 *
 * RecovCheckHost --
 *
 *	This decides if we should check up on a host.  If there has
 *	been recent message traffic there is no need to ping now,
 *	but we should check again later.  If there has been no
 *	message traffic our caller should ping.  Finally, if
 *	there are no reboot callbacks associated with the host,
 *	then we are not interested anymore.  Thus there are three
 *	values to return.
 *
 * Results:
 *	-1 if we are no longer interested in the host.
 *	0 if the host is presumably up and we don't have to ping.
 *	1 if our caller should ping.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ENTRY int
RecovCheckHost(spriteID)
    int	spriteID;
{
    register Hash_Entry *hashPtr;
    register RecovHostState *hostPtr = (RecovHostState *)NIL;
    register int check = -1;	/* forget about the host */
    register int state;

    LOCK_MONITOR;

    hashPtr = Hash_LookOnly(recovHashTable, (Address)spriteID);
    if (hashPtr != (Hash_Entry *)NIL) {
	hostPtr = (RecovHostState *)hashPtr->value;
	if ((hostPtr != (RecovHostState *)NIL) &&
	    (!List_IsEmpty(&hostPtr->rebootList))) {
	    state = hostPtr->state &
	 (RECOV_HOST_ALIVE|RECOV_HOST_BOOTING|RECOV_HOST_DYING|RECOV_HOST_DEAD);
	    if (state & (RECOV_HOST_ALIVE|RECOV_HOST_BOOTING)) {
		/*
		 * Check for recent message traffic before admitting
		 * that the other machine is up.
		 */
		Time time;
		Timer_GetTimeOfDay(&time, (int *)NIL, (Boolean *)NIL);
		Time_Subtract(time, hostPtr->time, &time);
		if (Time_GT(time, time_TenSeconds)) {
		    check = 1;	/* ping the host now */
		} else {
		    check = 0;	/* ping the host maybe next time */
		}
	    } else if (state & (RECOV_HOST_DEAD|RECOV_HOST_DYING)) {
		check = 1;	/* ping the host now */
	    }
	}
    }
    if (check < 0 && hostPtr != (RecovHostState *)NIL) {
	hostPtr->state &= ~RECOV_PINGING_HOST;
    }
    UNLOCK_MONITOR;
    return(check);
}

/*
 *----------------------------------------------------------------------
 *
 * Recov_GetStats --
 *
 *	Return the Recov_Stats to user-level, and perhaps more information
 *	about our internal opinion of other hosts.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Copies data out to user-space.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Recov_GetStats(size, userAddr)
    int size;
    Address userAddr;
{
    ReturnStatus status;
    int extraSpace = -1;

    if (size <= 0) {
	return(GEN_INVALID_ARG);
    }
    /*
     * See if the caller wants more than just statistics.
     */
    if (size > sizeof(Recov_Stats)) {
	extraSpace = size - sizeof(Recov_Stats);
	size = sizeof(Recov_Stats);
    }
    status = Vm_CopyOut(size, (Address)&recov_Stats, userAddr);

#ifdef notdef
    if (extraSpace > sizeof(int)) {
	/*
	 * Fill the user-space buffer with a count of hosts,
	 * and then information about each host.
	 */
	userAddr += sizeof(Recov_Stats);
	status = Recov_DumpState(extraSpace, userAddr);
    }
#endif notdef
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Recov_DumpState --
 *
 *	Dump internal state to user-level.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Copies data out to user-space.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Recov_DumpState(size, userAddr)
    int size;
    Address userAddr;
{
    ReturnStatus status;
    int numHosts, maxHosts;
    int *countPtr;
    int spriteID;
    Recov_State recovState;

    /*
     * We return a count, plus count number of Recov_State structures.
     */
    maxHosts = (size - sizeof(int)) / sizeof(Recov_State);
    countPtr = (int *)userAddr;
    if ((maxHosts == 0) && (size > sizeof(int))) {
	status = Vm_CopyOut(sizeof(int), (Address)&maxHosts, countPtr);
	return(status);
    }
     userAddr += sizeof(int);
    /*
     * Brute force.  Run through til MAX_HOSTS and try to grab
     * the state from the hash table.
     */
    numHosts = 0;
    for (spriteID = 1 ; spriteID < NET_NUM_SPRITE_HOSTS ; spriteID++) {
	if (Recov_GetHostInfo(spriteID, &recovState)) {
	    status = Vm_CopyOut(sizeof(recovState), (Address)&recovState,
			    userAddr);
	    if (status != SUCCESS) {
		return(status);
	    }
	    userAddr += sizeof(recovState);
	    numHosts++;
	    if (numHosts >= maxHosts) {
		break;
	    }
	}
    }

    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Recov_GetHostInfo --
 *
 *	Get the internal state about a host.
 *
 * Results:
 *	Fills in a Recov_State structure and returns TRUE,
 *	otherwise, if we don't know about the host, returns FALSE
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ENTRY Boolean
Recov_GetHostInfo(spriteID, recovStatePtr)
    int spriteID;
    Recov_State *recovStatePtr;
{
    Hash_Entry *hashPtr;
    RecovHostState *hostPtr;
    Boolean found = FALSE;

    LOCK_MONITOR;

    if (spriteID <= 0 || spriteID == rpc_SpriteID) {
	goto exit;
    } else {
	hashPtr = Hash_LookOnly(recovHashTable, (Address)spriteID);
	if (hashPtr == (Hash_Entry *)NULL || hashPtr->value == (Address)NIL) {
	    goto exit;
	} else {
	    hostPtr = (RecovHostState *)hashPtr->value;
	}
	recovStatePtr->spriteID = spriteID;
	recovStatePtr->state = hostPtr->state;
	recovStatePtr->clientState = hostPtr->clientState;
	recovStatePtr->bootID = hostPtr->bootID;
	recovStatePtr->time = hostPtr->time;
	found = TRUE;
    }
exit:
    UNLOCK_MONITOR;
    return(found);
}


/*
 *----------------------------------------------------------------------
 *
 * Recov_PrintTraceRecord --
 *
 *	Format and print the client data part of a recovery trace record.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	printf to the display.
 *
 *----------------------------------------------------------------------
 */
void
Recov_PrintTraceRecord(clientData, event, printHeaderFlag)
    ClientData clientData;	/* Client data in the trace record */
    int event;			/* Type, or event, from the trace record */
    Boolean printHeaderFlag;	/* If TRUE, a header line is printed */
{
    RecovTraceRecord *recPtr = (RecovTraceRecord *)clientData;
    char *name;
    if (printHeaderFlag) {
	/*
	 * Print column headers and a newline.
	 */
	printf("%10s %10s %17s\n", "Host", "State", "Event ");
    }
    if (clientData != (ClientData)NIL) {
	Net_SpriteIDToName(recPtr->spriteID, &name);
	if (name == (char *)NIL) {
	    printf("%10d ", recPtr->spriteID);
	} else {
	    printf("%10s ", name);
	}
	printf("%-8s", GetState(recPtr->state));
	printf("%3s", (recPtr->state & RECOV_CRASH_CALLBACKS) ?
			    " C " : "   ");
	printf("%3s", (recPtr->state & RECOV_PINGING_HOST) ?
			    " P " : "   ");
	printf("%3s", (recPtr->state & RECOV_REBOOT_CALLBACKS) ?
			    " R " : "   ");
	printf("%3s", (recPtr->state & RECOV_WANT_RECOVERY) ?
			    " W " : "   ");
	switch(event) {
	    case RECOV_CUZ_WAIT:
		printf("waiting");
		break;
	    case RECOV_CUZ_WAKEUP:
		printf("wakeup");
		break;
	    case RECOV_CUZ_INIT:
		printf("init");
		break;
	    case RECOV_CUZ_REBOOT:
		printf("reboot");
		break;
	    case RECOV_CUZ_CRASH:
		printf("crash");
		break;
	    case RECOV_CUZ_CRASH_UNDETECTED:
		printf("crash undetected");
		break;
	    case RECOV_CUZ_DONE:
		printf("done");
		break;
	    case RECOV_CUZ_PING_ASK:
		printf("ping (ask)");
		break;
	    case RECOV_CUZ_PING_CHK:
		printf("ping (check)");
		break;
	    case RECOV_TRACE_FS_STALE:
		printf("stale FS handle");
		break;
	    default:
		printf("(%x)", event);
		break;
	}
	/* Our caller prints a newline */
    }
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * Recov_PrintTrace --
 *
 *	Dump out the recovery trace.  Called via a console L1 keystroke.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints to the console.
 *
 *----------------------------------------------------------------------
 */

void
Recov_PrintTrace(numRecs)
    int numRecs;
{
    if (numRecs <= 0 || numRecs > recovTraceLength) {
	numRecs = recovTraceLength;
    }
    printf("RECOVERY TRACE\n");
    (void)Trace_Print(recovTraceHdrPtr, numRecs, Recov_PrintTraceRecord);
    Recov_PrintState();
    RecovPrintPingList();
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * Recov_PrintState --
 *
 *	Dump out the recovery state.  Called via a console L1 keystroke.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints to the console.
 *
 *----------------------------------------------------------------------
 */

void
Recov_PrintState()
{
    Hash_Search			hashSearch;
    register Hash_Entry		*hashEntryPtr;
    register RecovHostState	*hostPtr;
    char			*hostName;
    Time_Parts			timeParts;
    Time			bootTime;
    int				localOffset; /* minute offset for our tz */
    Time			currentTime;

    printf("RECOVERY STATE\n");
    Hash_StartSearch(&hashSearch);
    for (hashEntryPtr = Hash_Next(recovHashTable, &hashSearch);
	 hashEntryPtr != (Hash_Entry *)NIL;
	 hashEntryPtr = Hash_Next(recovHashTable, &hashSearch)) {
	hostPtr = (RecovHostState *)hashEntryPtr->value;
	if (hostPtr != (RecovHostState *)NIL) {

	    Net_SpriteIDToName(hostPtr->spriteID, &hostName);
	    printf("%-14s %-8s", hostName, GetState(hostPtr->state));
	    printf(" bootID 0x%8x", hostPtr->bootID);

	    /*
	     * Print out boot time in our timezone.
	     */
	    Timer_GetTimeOfDay(&currentTime, &localOffset, (Boolean *) NIL);
	    bootTime.seconds = hostPtr->bootID;
	    bootTime.microseconds = 0;
	    bootTime.seconds += (localOffset * 60);
	    Time_ToParts(bootTime.seconds, FALSE, &timeParts);
	    timeParts.month++;	/* So Jan is 1, not 0 */
	    printf(" %d/%d/%d %d:%02d:%02d ", timeParts.month,
		    timeParts.dayOfMonth,
		    timeParts.year, timeParts.hours, timeParts.minutes,
		    timeParts.seconds);

	    /*
	     * Print seconds ago we last heard from host.
	     */
	    printf("    %d ", currentTime.seconds - hostPtr->time.seconds);
	    PrintExtraState(hostPtr);
	    printf("\n");
	}
    }
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * GetState --
 *
 *	Return a printable string for the host's state.
 *
 * Results:
 *	A pointer to a string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static char *
GetState(state)
    int state;
{
    switch(state & (RECOV_HOST_ALIVE|RECOV_HOST_DYING|RECOV_HOST_DEAD|
		    RECOV_HOST_BOOTING)) {
	default:
	case RECOV_STATE_UNKNOWN:
	    return("Unknown");
	case RECOV_HOST_ALIVE:
	    return("Alive");
	case RECOV_HOST_BOOTING:
	    return("Booting");
	case RECOV_HOST_DYING:
	    return("Dying");
	case RECOV_HOST_DEAD:
	    return("Dead");
    }
}

/*
 *----------------------------------------------------------------------
 *
 * RecovExtraState --
 *
 *	Prints out strings for various auxilliary state bits.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints out stuff.
 *
 *----------------------------------------------------------------------
 */

static void
PrintExtraState(hostPtr)
    RecovHostState *hostPtr;
{
    if (hostPtr->state & RECOV_CRASH_CALLBACKS) {
	printf("Crash callbacks ");
    }
    if (hostPtr->state & RECOV_WANT_RECOVERY) {
	printf("Want recovery ");
    }
    if (hostPtr->state & RECOV_REBOOT_CALLBACKS) {
	printf("Reboot callbacks ");
    }
    if (hostPtr->state & RECOV_FAILURE) {
	printf("Failure ");
    }
    if (hostPtr->clientState & CLT_RECOV_IN_PROGRESS) {
	printf("Clt-inprogress ");
    }
    if (hostPtr->clientState & SRV_RECOV_IN_PROGRESS) {
	printf("Srv-inprogress ");
    }
}


void
Recov_ChangePrintLevel(newLevel)
    int	newLevel;
{
    recov_PrintLevel = newLevel;
    return;
}
