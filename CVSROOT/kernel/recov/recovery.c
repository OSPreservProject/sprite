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
#endif not lint


#include "sprite.h"
#include "recov.h"
#include "sync.h"
#include "net.h"
#include "rpc.h"
#include "hash.h"
#include "mem.h"
#include "trace.h"

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
int recov_CrashDelay;

/*
 * When a crash call back is avoided because the host didn't really go
 * down we increment the following counter.
 */
int recovNumNonCrashes = 0;

/*
 * The state of other hosts is kept in a hash table keyed on SpriteID.
 * This state is maintained by Recov_HostAlive and Recov_HostDead, which are
 * called in turn after packet reception or RPC timeout, respectively.
 * Recov_HostDead is also called by the Rpc_Daemon if it can't get an
 * explicit acknowledgment from a client.
 */
static Hash_Table	recovHashTableStruct;
static Hash_Table	*recovHashTable = &recovHashTableStruct;

typedef struct RecovHostState {
    int			state;		/* flags defined below */
    int			clientState;	/* flags defined in recov.h */
    int			spriteID;	/* Sprite Host ID */
    int			bootID;		/* Boot timestamp from RPC header */
    Time		time;		/* Time of last message */
    Sync_Condition	alive;		/* Notified when host comes up */
    Sync_Condition	recovery;	/* Notified when recovery is complete */
    List_Links		rebootList;	/* List of callbacks for when this
					 * host reboots. */
} RecovHostState;

#define RECOV_INIT_HOST(hostPtr, zspriteID, zstate, zbootID) \
    hostPtr = Mem_New(RecovHostState); \
    Byte_Zero(sizeof(RecovHostState), (Address)hostPtr); \
    List_Init(&(hostPtr)->rebootList); \
    (hostPtr)->spriteID = zspriteID; \
    (hostPtr)->state = zstate; \
    (hostPtr)->bootID = zbootID;

/*
 * Access to the hash table is monitored.
 */
static Sync_Lock recovLock;
#define LOCKPTR (&recovLock)

/*
 * Host state:
 *	RECOV_STATE_UNKNOWN	Initial state.
 *	RECOV_HOST_ALIVE	Set when we receive a message from the host
 *	RECOV_HOST_DYING	Set when an RPC times out.
 *	RECOV_HOST_DEAD		Set when crash callbacks have been started.
 *	RECOV_HOST_BOOTING	Set in the limbo period when a host is booting
 *				and talking to the world, but isn't ready
 *				for full recovery actions yet.
 *
 *	RECOV_CRASH_CALLBACKS	Set during the crash call-backs, this is used
 *				to block RPC server processes until the
 *				crash recovery actions have completed.
 *	RECOV_WANT_RECOVERY	Set if another module wants a callback at reboot
 *	RECOV_PINGING_HOST	Set while we ping a host to see when it reboots
 *	RECOV_REBOOT_CALLBACKS	Set while reboot callbacks are pending.	
 *
 *	RECOV_WAITING		artificial state to trace Rpc_WaitForHost
 *	RECOV_CRASH		artificial state to trace RecovCrashCallBacks
 *	RECOV_REBOOT		artificial state to trace RecovRebootCallBacks
 */
#define RECOV_STATE_UNKNOWN	0x0
#define RECOV_HOST_ALIVE	0x1
#define RECOV_HOST_DYING	0x2
#define RECOV_HOST_DEAD		0x4
#define RECOV_HOST_BOOTING	0x10

#define RECOV_CRASH_CALLBACKS	0x0100
#define RECOV_WANT_RECOVERY	0x0200
#define RECOV_PINGING_HOST	0x0400
#define RECOV_REBOOT_CALLBACKS	0x0800

#define RECOV_WAITING		0x10
#define RECOV_CRASH		0x20
#define RECOV_REBOOT		0x40

/*
 * A host is "pinged" (to see when it reboots) at an interval determined by
 * rpcPingSeconds.
 */
int recovPingSeconds = 30;



/*
 * A trace is kept for debugging/understanding the host state transisions.
 */
typedef struct RecovTraceRecord {
    int		spriteID;		/* Host ID whose state changed */
    int		state;			/* Their new state */
} RecovTraceRecord;

/*
 * Tracing events, these describe the trace record.  Note that some
 *	trace types are defined in rpc.h for use with Rpc_HostTrace.
 *
 *	RECOV_CUZ_WAIT		Wait in Rpc_WaitForHost
 *	RECOV_CUZ_WAKEUP	Wakeup in Rpc_WaitForHost
 *	RECOV_CUZ_INIT		First time we were interested in the host
 *	RECOV_CUZ_REBOOT	We detected a reboot
 *	RECOV_CUZ_CRASH		We detected a crash
 *	RECOV_CUZ_DONE		Recovery actions completed
 *	RECOV_CUZ_PING_CHK	We are pinging the host to check it out
 *	RECOV_CUZ_PING_ASK	We are pinging the host because we were asked
 */
#define RECOV_CUZ_WAIT		0x1
#define RECOV_CUZ_WAKEUP	0x2
#define RECOV_CUZ_INIT		0x4
#define RECOV_CUZ_REBOOT	0x8
#define RECOV_CUZ_CRASH		0x10
#define RECOV_CUZ_DONE		0x20
#define RECOV_CUZ_PING_CHK	0x40
#define RECOV_CUZ_PING_ASK	0x80

Trace_Header recovTraceHdr;
Trace_Header *recovTraceHdrPtr = &recovTraceHdr;
int recovTraceLength = 50;
Boolean recovTracing = TRUE;

#ifndef CLEAN

#define RECOV_TRACE(zspriteID, zstate, event) \
    if (recovTracing) {\
	RecovTraceRecord rec;\
	rec.spriteID = zspriteID;\
	rec.state = zstate;\
	Trace_Insert(recovTraceHdrPtr, event, &rec);\
    }
#else

#define RECOV_TRACE(zspriteID, zstate, event)

#endif not CLEAN
/*
 * Forward declarations.
 */
void RecovRebootCallBacks();
void RecovCrashCallBacks();
void RecovDelayedCrashCallBacks();
void CallBacksDone();
void MarkRecoveryComplete();
void MarkHostDead();
int  GetHostState();
void GetRebootList();
void CheckHost();


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
    Hash_Init(recovHashTable, 8, HASH_ONE_WORD_KEYS);
    List_Init(&crashCallBackList);
    Trace_Init(recovTraceHdrPtr, recovTraceLength,
		sizeof(RecovTraceRecord), 0);
    recov_CrashDelay = timer_IntOneHour;
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

    notifyPtr = Mem_New(NotifyElement);
    notifyPtr->proc = crashCallBackProc;
    notifyPtr->data = crashData;
    List_InitElement((List_Links *) notifyPtr);
    List_Insert((List_Links *) notifyPtr, LIST_ATREAR(&crashCallBackList));
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
    int bootID;			/* Boot time stamp from message header */
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
    Boolean reboot = FALSE;	/* Used to control print statements at reboot */
    register state;

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

    hashPtr = Hash_Find(recovHashTable, spriteID);
    if (hashPtr->value == (Address)NIL) {
	/*
	 * Initialize the host's state. This is the first time we've talked
	 * to it since we've been up, so take no action.
	 */
	RECOV_INIT_HOST(hostPtr, spriteID, RECOV_HOST_ALIVE, bootID);
	hashPtr->value = (Address)hostPtr;

	Net_HostPrint(spriteID, "is up\n");
	RECOV_TRACE(spriteID, RECOV_HOST_ALIVE, RECOV_CUZ_INIT);
    } else {
	hostPtr = (RecovHostState *)hashPtr->value;
    }
    state = hostPtr->state;
    /*
     * Have to read the clock in order to suppress repeated pings,
     * see GetHostState and Recov_IsHostDown.
     */
    Timer_GetTimeOfDay(&hostPtr->time, (int *)NIL, (Boolean *)NIL);
    /*
     * Check for a rebooted peer by comparing boot time stamps.
     */
    if (hostPtr->bootID != bootID) {
	if (hostPtr->bootID != 0) {
	    Net_HostPrint(spriteID, "rebooted\n");
	    reboot = TRUE;
	} else {
	    /*
	     * We initialized state before talking to the host the first time.
	     * The state is 'unknown' so we won't do crash call-backs.
	     */
	}
	hostPtr->bootID = bootID;
	RECOV_TRACE(spriteID, state, RECOV_CUZ_REBOOT);
	if (state & (RECOV_HOST_ALIVE|RECOV_HOST_DYING|RECOV_HOST_BOOTING)) {
	    /*
	     * A crash occured un-detected.  We do the crash call-backs
	     * first, and block server processes in the meantime.
	     * RECOV_CRASH_CALLBACKS flag is cleared by RecovCrashCallBacks.
	     */
	    RECOV_TRACE(spriteID, RECOV_CRASH, RECOV_CUZ_REBOOT);
	    state &= ~(RECOV_HOST_ALIVE|RECOV_HOST_DYING);
	    state |= RECOV_HOST_BOOTING;
	    if ((state & RECOV_CRASH_CALLBACKS) == 0) {
		state |= RECOV_CRASH_CALLBACKS;
		Proc_CallFunc(RecovCrashCallBacks, spriteID, 0);
	    }
	}
    } else  if ( !(state & RECOV_CRASH_CALLBACKS) &&
		(state & RECOV_HOST_ALIVE)) {
	/*
	 * Fast path.  We already think the other host is up, it didn't
	 * reboot, and there are no pending crash call-backs to 
	 * synchronize with.
	 */
	goto exit;
    }
    /*
     * Block servers until crash recovery actions complete.
     * This prevents servicing requests from clients until after the
     * recovery actions complete.
     */
    if (! asyncRecovery) {
	hostPtr->state = state;
	while (hostPtr->state & RECOV_CRASH_CALLBACKS) {
	    Sync_Wait(&hostPtr->recovery, FALSE);
	    if (sys_ShuttingDown) {
		Sys_Printf("Warning, Server exiting Recov_HostAlive\n");
		UNLOCK_MONITOR;
		Proc_Exit(1);
	    }
	}
    }
    state = hostPtr->state;
    /*
     * Now that we've taken care of crash recovery, we see if the host
     * is newly up.  If so, invoke any reboot call-backs and notify
     * waiting processes. This means clientA (us) may start
     * re-opening files from serverB (the other guy) at the same time
     * as clientA (us) is closing files that serverB had had open.
     * ie. both the crash and reboot call backs may proceed in parallel.
     */
    switch(state &
       (RECOV_HOST_ALIVE|RECOV_HOST_BOOTING|RECOV_HOST_DEAD|RECOV_HOST_DYING)) {
        case RECOV_STATE_UNKNOWN:	/* This is zero, no bits set */
	    /*
	     * We have uninitialized state for the host, mark it alive.
	     */
	    Net_HostPrint(spriteID, "is up\n");
	    if (rpcNotActive) {
		state |= RECOV_HOST_BOOTING;
	    } else {
		state |= RECOV_HOST_ALIVE;
	    }
	    break;
	case RECOV_HOST_ALIVE:
	    /*
	     * Host already alive.
	     */
	    break;
	case RECOV_HOST_BOOTING:
	    /*
	     * See if a booting host is ready yet.
	     */
	    if (! rpcNotActive) {
		state &= ~RECOV_HOST_BOOTING;
		state |= RECOV_HOST_ALIVE|RECOV_WANT_RECOVERY;
	    }
	    break;
	case RECOV_HOST_DYING:
	case RECOV_HOST_DEAD:
	    /*
	     * See if the host is newly booting or back from a net partition.
	     */
	    if ( !reboot ) {
		Net_HostPrint(spriteID, "is back again\n");
	    }
	    if (rpcNotActive) {
		state |= RECOV_HOST_BOOTING;
	    } else {
		hostPtr->state |= RECOV_HOST_ALIVE;
		if (reboot || (hostPtr->state & RECOV_HOST_DEAD)) {
		    state |= RECOV_WANT_RECOVERY;
		}
	    }
	    state &= ~(RECOV_HOST_DEAD|RECOV_HOST_DYING);
	    break;
	default:
	    Sys_Panic(SYS_WARNING, "Unexpected recovery state <%x> for ",
		    state);
	    Net_HostPrint(spriteID, "\n");
	    break;
    }
    /*
     * After a host comes up enough to support RPC service, we
     * initiate reboot recovery if needed.
     */
    if ((state & RECOV_HOST_ALIVE) &&
	(state & RECOV_WANT_RECOVERY) &&
	(state & RECOV_REBOOT_CALLBACKS) == 0) {
	state &= ~RECOV_WANT_RECOVERY;
	state |= RECOV_REBOOT_CALLBACKS;
	Proc_CallFunc(RecovRebootCallBacks, spriteID, 0);
    }
    hostPtr->state = state;
exit:
    UNLOCK_MONITOR;
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
	 */
	UNLOCK_MONITOR;
	return;
    }

    hashPtr = Hash_Find(recovHashTable, spriteID);
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
	    hostPtr->state |= RECOV_HOST_DYING;
	    Net_HostPrint(spriteID, "is apparently down\n");
	    Proc_CallFunc(RecovDelayedCrashCallBacks, spriteID,
			    recov_CrashDelay);
#ifdef the_old_way
	    hostPtr->state |= RECOV_HOST_DEAD|RECOV_CRASH_CALLBACKS;
	    Net_HostPrint(spriteID, "is down\n");
	    RECOV_TRACE(spriteID, hostPtr->state, RECOV_CUZ_CRASH);
	    Proc_CallFunc(RecovCrashCallBacks, spriteID, 0);
#endif the_old_way
	    break;
    }
    UNLOCK_MONITOR;
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
	Sys_Panic(SYS_WARNING, "Recov_IsHostDown, got broadcast address\n");
	return(SUCCESS);
    }
    switch (GetHostState(spriteID)) {
	case RECOV_STATE_UNKNOWN:
	    RECOV_TRACE(spriteID, RECOV_STATE_UNKNOWN, RECOV_CUZ_PING_ASK);
	    status = Rpc_Ping(spriteID);
	    break;
	case RECOV_HOST_BOOTING:
	case RECOV_HOST_ALIVE:
	case RECOV_HOST_DYING:	/* fake it to allow for the grace period */
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
	Sys_Panic(SYS_FATAL, "Recov_RebootRegister, bad hostID %d\n", spriteID);
    } else {
	hashPtr = Hash_Find(recovHashTable, spriteID);
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
	    notifyPtr = Mem_New(NotifyElement);
	    notifyPtr->proc = rebootCallBackProc;
	    notifyPtr->data = rebootData;
	    List_InitElement((List_Links *)notifyPtr);
	    List_Insert((List_Links *)notifyPtr,
			LIST_ATFRONT(&hostPtr->rebootList));
	}
	/*
	 * Start a perpetual call back so we note crashes.
	 */
	if ((hostPtr->state & RECOV_PINGING_HOST) == 0) {
	    hostPtr->state |= RECOV_PINGING_HOST;
	    Proc_CallFunc(CheckHost, spriteID, 0);
	}
    }
    UNLOCK_MONITOR;
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
    LOCK_MONITOR;

    RECOV_TRACE(spriteID, RECOV_STATE_UNKNOWN, event);

    UNLOCK_MONITOR;
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

    hashPtr = Hash_LookOnly(recovHashTable, spriteID);
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
 *	client state word.
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

ENTRY void
Recov_SetClientState(spriteID, stateBits)
    int spriteID;
    int stateBits;
{
    Hash_Entry *hashPtr;
    RecovHostState *hostPtr;

    LOCK_MONITOR;

    hashPtr = Hash_Find(recovHashTable, spriteID);
    hostPtr = (RecovHostState *)hashPtr->value;
    if (hostPtr == (RecovHostState *)NIL) {
	RECOV_INIT_HOST(hostPtr, spriteID, RECOV_STATE_UNKNOWN, 0);
	hashPtr->value = (Address)hostPtr;
    }
    hostPtr->clientState |= stateBits;
    UNLOCK_MONITOR;
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
    register Hash_Entry *hashPtr;
    register RecovHostState *hostPtr;

    LOCK_MONITOR;

    hashPtr = Hash_LookOnly(recovHashTable, spriteID);
    if (hashPtr != (Hash_Entry *)NIL) {
	hostPtr = (RecovHostState *)hashPtr->value;
	if (hostPtr != (RecovHostState *)NIL) {
	    hostPtr->clientState &= ~stateBits;
	}
    }
    UNLOCK_MONITOR;
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

void
RecovRebootCallBacks(data, callInfoPtr)
    ClientData data;
    Proc_CallInfo *callInfoPtr;
{
    ReturnStatus status;
    List_Links notifyList;
    register NotifyElement *notifyPtr;
    register int spriteID = (int)data;

    GetRebootList(&notifyList, spriteID);
    while (!List_IsEmpty(&notifyList)) {
	notifyPtr = (NotifyElement *)List_First(&notifyList);
	(*notifyPtr->proc)(spriteID, notifyPtr->data);
	List_Remove(notifyPtr);
	Mem_Free(notifyPtr);
    }
    CallBacksDone(spriteID);
}

/*
 *----------------------------------------------------------------------
 *
 * RecovCrashCallBacks --
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

void
RecovCrashCallBacks(data, callInfoPtr)
    ClientData data;
    Proc_CallInfo *callInfoPtr;
{
    register NotifyElement *notifyPtr;
    register int spriteID = (int)data;

    LIST_FORALL(&crashCallBackList, (List_Links *)notifyPtr) {
	if (notifyPtr->proc != (void (*)())NIL) {
	    (*notifyPtr->proc)(spriteID, notifyPtr->data);
	 }
    }
    MarkRecoveryComplete(spriteID);
    RECOV_TRACE(spriteID, RECOV_CRASH, RECOV_CUZ_DONE);
    callInfoPtr->interval = 0;	/* Don't call again */
}

/*
 *----------------------------------------------------------------------
 *
 * RecovDelayedCrashCallBacks --
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

void
RecovDelayedCrashCallBacks(data, callInfoPtr)
    ClientData data;
    Proc_CallInfo *callInfoPtr;
{
    register NotifyElement *notifyPtr;
    register int spriteID = (int)data;
    int state;

    state = GetHostState(spriteID);
    if (state & RECOV_HOST_DYING) {
	Net_HostPrint(spriteID, "considered dead\n");
	MarkHostDead(spriteID);
	LIST_FORALL(&crashCallBackList, (List_Links *)notifyPtr) {
	    if (notifyPtr->proc != (void (*)())NIL) {
		(*notifyPtr->proc)(spriteID, notifyPtr->data);
	     }
	}
    } else if ((state & RECOV_HOST_DEAD) == 0) {
	recovNumNonCrashes++;
    }
    callInfoPtr->interval = 0;	/* Don't call again */
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
{
    register Hash_Entry *hashPtr;
    register RecovHostState *hostPtr;

    LOCK_MONITOR;

    hashPtr = Hash_LookOnly(recovHashTable, spriteID);
    if (hashPtr != (Hash_Entry *)NIL) {
	hostPtr = (RecovHostState *)hashPtr->value;
	if (hostPtr != (RecovHostState *)NIL) {
	    hostPtr->state &= ~RECOV_CRASH_CALLBACKS;
	    Sync_Broadcast(&hostPtr->recovery);
	}
    }
    UNLOCK_MONITOR;
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
{
    register Hash_Entry *hashPtr;
    register RecovHostState *hostPtr;

    LOCK_MONITOR;

    hashPtr = Hash_LookOnly(recovHashTable, spriteID);
    if (hashPtr != (Hash_Entry *)NIL) {
	hostPtr = (RecovHostState *)hashPtr->value;
	if (hostPtr != (RecovHostState *)NIL) {
	    hostPtr->state &= ~RECOV_HOST_DYING;
	    hostPtr->state |= RECOV_HOST_DEAD;
	}
    }
    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * GetHostState --
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
GetHostState(spriteID)
    int spriteID;
{
    register Hash_Entry *hashPtr;
    register RecovHostState *hostPtr;
    register int state = RECOV_STATE_UNKNOWN;
    Time time;

    LOCK_MONITOR;

    hashPtr = Hash_LookOnly(recovHashTable, spriteID);
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
 * CheckHost --
 *
 *	This is a periodic check on the state of another host.  This pings
 *	the remote host if it's down or there hasn't been recent traffic.
 *	A side effect of a successful ping is a call to Recov_HostAlive which
 *	triggers the recovery actions.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	This will pings the host unless there has been recent message
 *	traffic.  It reschedules itself perpetually.
 *
 *----------------------------------------------------------------------
 */

static void
CheckHost(data, callInfoPtr)
    ClientData data;
    Proc_CallInfo *callInfoPtr;
{
    register int spriteID = (int)data;
    register int state;

    state = GetHostState(spriteID);
    switch (state) {
	case RECOV_HOST_DEAD:
	case RECOV_HOST_BOOTING:
	case RECOV_STATE_UNKNOWN:
	    RECOV_TRACE(spriteID, state, RECOV_CUZ_PING_CHK);
	    (void) Rpc_Ping(spriteID);
	    break;
	case RECOV_HOST_ALIVE:
	    break;
    }
    callInfoPtr->interval = recovPingSeconds * timer_IntOneSecond;
}

/*
 *----------------------------------------------------------------------
 *
 * GetRebootList --
 *
 *	Copy out the list of reboot callbacks.  The list is protected by
 * 	a monitor, but we don't want to call any recovery procedures from
 *	inside that monitor.
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

ENTRY void
GetRebootList(notifyListHdr, spriteID)
    List_Links *notifyListHdr;
    int spriteID;
{
    register Hash_Entry *hashPtr;
    register RecovHostState *hostPtr;
    register NotifyElement *notifyPtr;
    register NotifyElement *newNotifyPtr;

    LOCK_MONITOR;

    hashPtr = Hash_LookOnly(recovHashTable, spriteID);
    hostPtr = (RecovHostState *)hashPtr->value;
    List_Init(notifyListHdr);
    LIST_FORALL(&hostPtr->rebootList, (List_Links *)notifyPtr) {
	newNotifyPtr = Mem_New(NotifyElement);
	newNotifyPtr->proc = notifyPtr->proc;
	newNotifyPtr->data = notifyPtr->data;
	List_InitElement((List_Links *)newNotifyPtr);
	List_Insert((List_Links *)newNotifyPtr, LIST_ATREAR(notifyListHdr));
    }
    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * CallBacksDone --
 *
 *	Clear the internal state bit that says callbacks are in progress.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	As above.
 *
 *----------------------------------------------------------------------
 */

ENTRY void
CallBacksDone(spriteID)
    int spriteID;
{
    register Hash_Entry *hashPtr;
    register RecovHostState *hostPtr;

    LOCK_MONITOR;

    hashPtr = Hash_LookOnly(recovHashTable, spriteID);
    hostPtr = (RecovHostState *)hashPtr->value;
    if ((hostPtr->state & RECOV_REBOOT_CALLBACKS) == 0) {
	Sys_Panic(SYS_WARNING, "RecovCallBacksDone found bad state\n");
    }
    hostPtr->state &= ~RECOV_REBOOT_CALLBACKS;
    UNLOCK_MONITOR;
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
 *	Sys_Printf to the display.
 *
 *----------------------------------------------------------------------
 */
int
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
	Sys_Printf("%10s %10s %17s\n", "Host", "State", "Event ");
    }
    if (clientData != (ClientData)NIL) {
	Net_SpriteIDToName(recPtr->spriteID, &name);
	if (name == (char *)NIL) {
	    Sys_Printf("%10d ", recPtr->spriteID);
	} else {
	    Sys_Printf("%10s ", name);
	}
	switch(recPtr->state &
		    ~(RECOV_CRASH_CALLBACKS|RECOV_PINGING_HOST|
		      RECOV_REBOOT_CALLBACKS|RECOV_WANT_RECOVERY)) {
	    case RECOV_STATE_UNKNOWN:
		Sys_Printf("%-8s", "Unknown");
		break;
	    case RECOV_HOST_ALIVE:
		Sys_Printf("%-8s ", "Alive");
		break;
	    case RECOV_HOST_DYING:
		Sys_Printf("%-8s ", "Dying");
		break;
	    case RECOV_HOST_DEAD:
		Sys_Printf("%-8s ", "Dead");
		break;
	    case RECOV_WAITING:
		Sys_Printf("%-8s ", "Waiting");
		break;
	    case RECOV_CRASH:
		Sys_Printf("%-8s ", "Crash callbacks");
		break;
	    case RECOV_REBOOT:
		Sys_Printf("%-8s ", "Reboot callbacks");
		break;
	}
	Sys_Printf("%3s", (recPtr->state & RECOV_CRASH_CALLBACKS) ?
			    " C " : "   ");
	Sys_Printf("%3s", (recPtr->state & RECOV_PINGING_HOST) ?
			    " P " : "   ");
	Sys_Printf("%3s", (recPtr->state & RECOV_REBOOT_CALLBACKS) ?
			    " R " : "   ");
	Sys_Printf("%3s", (recPtr->state & RECOV_WANT_RECOVERY) ?
			    " W " : "   ");
	switch(event) {
	    case RECOV_CUZ_WAIT:
		Sys_Printf("waiting");
		break;
	    case RECOV_CUZ_WAKEUP:
		Sys_Printf("wakeup");
		break;
	    case RECOV_CUZ_INIT:
		Sys_Printf("init");
		break;
	    case RECOV_CUZ_REBOOT:
		Sys_Printf("reboot");
		break;
	    case RECOV_CUZ_CRASH:
		Sys_Printf("crash");
		break;
	    case RECOV_CUZ_DONE:
		Sys_Printf("done");
		break;
	    case RECOV_CUZ_PING_ASK:
		Sys_Printf("ping (ask)");
		break;
	    case RECOV_CUZ_PING_CHK:
		Sys_Printf("ping (check)");
		break;
	    case RECOV_TRACE_FS_STALE:
		Sys_Printf("stale FS handle");
		break;
	    default:
		Sys_Printf("(%x)", event);
		break;
	}
	/* Our caller prints a newline */
    }
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
    Sys_Printf("RECOVERY TRACE\n");
    Trace_Print(recovTraceHdrPtr, numRecs, Recov_PrintTraceRecord);
}
