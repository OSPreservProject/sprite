/* 
 * recovProc.c --
 *
 *	Recov_Proc is the process that periodically checks the status
 *	of other hosts.  It is pretty simple, but its in its own file
 *	because of the monitor lock needed to synchronize access to
 *	its list of interesting hosts.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include "sprite.h"
#include "recov.h"
#include "sync.h"
#include "rpc.h"
#include "stdlib.h"

/*
 * A host is "pinged" (to see when it reboots) at an interval determined by
 * rpcPingSeconds.
 */
int recovPingSeconds = 30;
/*
 * Whether or not to ping at absolute intervals instead of 30 after the last
 * ping finished.  This is used for testing the affects of the synchronization
 * of client pinging on the servers.
 */
Boolean			recov_AbsoluteIntervals = TRUE;
Timer_QueueElement	recovIntervalElement;
unsigned int		recovPingEvent;
/*
 * A list of hosts to ping is used by Recov_Proc.
 */
typedef struct RecovPing {
    List_Links links;
    int spriteID;	/* The host we are interested in */
    Boolean active;	/* TRUE if we are really interested in it */
} RecovPing;

List_Links recovPingListHdr;
List_Links *recovPingList = &recovPingListHdr;
/*
 * Access to the ping list is monitored
 */
static Sync_Lock recovPingLock;
#define LOCKPTR (&recovPingLock)

static RecovPing *FirstHostToCheck();
static RecovPing *NextHostToCheck();
static void Deactivate();


/*
 *----------------------------------------------------------------------
 *
 * RecovPingInterval --
 *
 *	Set up the callback routine used for recov pinging.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
void
RecovPingInterval(timer, clientData)
    Timer_Ticks	time;
    ClientData	clientData;
{
    Sync_EventWakeup(recovPingEvent);
    Timer_ScheduleRoutine(&recovIntervalElement, TRUE);

    return;
}


/*
 *----------------------------------------------------------------------
 *
 * RecovPingInit --
 *
 *	Set up the data structures used by the recovery ping process.
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
RecovPingInit()
{
    Sync_LockInitDynamic(&recovPingLock, "Recov:pingListLock");
    List_Init(recovPingList);
    if (recov_AbsoluteIntervals) {
	recovIntervalElement.routine = RecovPingInterval;
	recovIntervalElement.clientData = 0;
	recovIntervalElement.interval =
		timer_IntOneSecond * recovPingSeconds;
	Timer_ScheduleRoutine(&recovIntervalElement, TRUE);
    }
    return;
}



/*
 *----------------------------------------------------------------------
 *
 * Recov_Proc --
 *
 *	Recovery process that periodically pings other hosts to verify
 *	that they are still and if they have rebooted since we last
 *	heard from them.
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
Recov_Proc()
{
    RecovPing *pingPtr;
    Time wait;
    int check;

    while (TRUE) {
	/*
	 * Calculate wait period inside loop so we can change it
	 * on the fly.
	 */
	if (!recov_AbsoluteIntervals) {
	    Time_Multiply(time_OneSecond, recovPingSeconds, &wait);
	    Sync_WaitTime(wait);
	} else {
	    (void) Sync_EventWait(recovPingEvent, FALSE);
	}
	if (sys_ShuttingDown) {
	    printf("Recov_Proc exiting.\n");
	    break;
	}
	/*
	 * Scan the ping list looking for hosts we should check.
	 * We'll ping them if there hasn't been recent message traffic,
	 * and we'll deactivate our interest if there is no set
	 * of reboot call backs associated with the host.
	 */
	pingPtr = FirstHostToCheck();
	while (pingPtr != (RecovPing *)NIL) {
	    check = RecovCheckHost(pingPtr->spriteID);
	    if (check > 0) {
		RECOV_TRACE(pingPtr->spriteID,
		    RecovGetLastHostState(pingPtr->spriteID),
		    RECOV_CUZ_PING_CHK);
		recov_Stats.pings++;
		Rpc_Ping(pingPtr->spriteID);
	    } else if (check == 0) {
		recov_Stats.pingsSuppressed++;
	    } else {
		Deactivate(pingPtr);
	    }
	    pingPtr = NextHostToCheck(pingPtr);
	}
    }
    Proc_Exit(0);
}

/*
 *----------------------------------------------------------------------
 *
 * FirstHostToCheck --
 *
 *	This pulls the first host to check off the ping list.
 *
 * Results:
 *	A pingPtr.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static RecovPing *
FirstHostToCheck()
{
    RecovPing *pingPtr;

    LOCK_MONITOR;

    if (List_IsEmpty(recovPingList)) {
	pingPtr = (RecovPing *)NIL;
    } else {
	pingPtr = (RecovPing *)List_First(recovPingList);
    }

    UNLOCK_MONITOR;
    return(pingPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * NextHostToCheck --
 *
 *	This pulls the next host to check off the ping list.
 *
 * Results:
 *	A pingPtr.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static RecovPing *
NextHostToCheck(pingPtr)
    RecovPing *pingPtr;
{
    LOCK_MONITOR;

    pingPtr = (RecovPing *)List_Next((List_Links *)pingPtr);
    if (List_IsAtEnd(recovPingList, (List_Links *)pingPtr)) {
	pingPtr = (RecovPing *)NIL;
    }

    UNLOCK_MONITOR;
    return(pingPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * RecovAddHostToPing --
 *
 *	Add an entry to the ping list.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	List_Insert.
 *
 *----------------------------------------------------------------------
 */

void
RecovAddHostToPing(spriteID)
    int spriteID;
{
    RecovPing *pingPtr;
    LOCK_MONITOR;

    LIST_FORALL(recovPingList, (List_Links *)pingPtr) {
	if (pingPtr->spriteID == spriteID) {
	    if (!pingPtr->active) {
		recov_Stats.numHostsPinged++;
		pingPtr->active = TRUE;
	    }
	    UNLOCK_MONITOR;
	    return;
	}
    }
    pingPtr = (RecovPing *)malloc(sizeof(RecovPing));
    recov_Stats.numHostsPinged++;
    pingPtr->active = TRUE;
    pingPtr->spriteID = spriteID;
    List_InitElement((List_Links *)pingPtr);
    List_Insert((List_Links *)pingPtr, LIST_ATREAR(recovPingList));

    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * Deactivate --
 *
 *	Deactivate an entry on the ping list.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	List_Insert.
 *
 *----------------------------------------------------------------------
 */

static void
Deactivate(pingPtr)
    RecovPing *pingPtr;
{
    LOCK_MONITOR;

    if (pingPtr->active) {
	recov_Stats.numHostsPinged--;
	pingPtr->active = FALSE;
    }

    UNLOCK_MONITOR;

}

ENTRY void
RecovPrintPingList()
{
    RecovPing *pingPtr;

    LOCK_MONITOR;

    if (List_IsEmpty(recovPingList)) {
	return;
    } 
    printf("\nPING_LIST\n");
    for (pingPtr = (RecovPing *)List_First(recovPingList);
	    !List_IsAtEnd(recovPingList, (List_Links *)pingPtr);
	    pingPtr = (RecovPing *)List_Next((List_Links *)pingPtr)) {
	printf("host %d is %s\n", pingPtr->spriteID,
		pingPtr->active ? "active" : "inactive");
    }
	 
    UNLOCK_MONITOR;

    return;
}
