/* 
 * procSysCall.c --
 *
 *	Short stubs for Sprite system calls.  These routines go between the 
 *	MIG-generated routine on the one side and the actual Sprite routine 
 *	on the other side.
 *
 * Copyright 1991, 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/sprited/proc/RCS/procSysCall.c,v 1.4 92/07/16 18:06:55 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <mach.h>
#include <proc.h>
#include <sig.h>
#include <spriteSrvServer.h>


/*
 *----------------------------------------------------------------------
 *
 * Proc_DetachStub --
 *
 *	Detach the current process from its parent.
 *
 * Results:
 *	Returns KERN_SUCCESS.  Proc_Detach always succeeds, so there's no 
 *	Sprite return status.  Fills in the "pending signals" flag.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
    
kern_return_t
Proc_DetachStub(serverPort, status, sigPendingPtr)
    mach_port_t serverPort;
    int status;			/* Detach status from caller */
    boolean_t *sigPendingPtr;	/* OUT: is there a signal pending */
{
#ifdef lint
    serverPort = serverPort;
#endif
    (void)Proc_Detach(status);
    *sigPendingPtr = Sig_Pending(Proc_GetCurrentProc());
    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_GetFamilyIDStub --
 *
 *	Get the process family ID (process group) for the given process.
 *
 * Results:
 *	Returns KERN_SUCCESS.  Fills in the Sprite status code from 
 *	Proc_GetFamilyID and the "pending signals" flag.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
Proc_GetFamilyIDStub(serverPort, pid, statusPtr, familyIdPtr,
		     sigPendingPtr)
    mach_port_t serverPort;
    Proc_PID pid;		/* the process to get the family for */
    ReturnStatus *statusPtr;	/* OUT: Sprite return status */
    Proc_PID *familyIdPtr;	/* OUT: the resulting family ID */
    boolean_t *sigPendingPtr;	/* OUT: is there a signal pending */
{
#ifdef lint
    serverPort = serverPort;
#endif
    *statusPtr = Proc_GetFamilyID(pid, familyIdPtr);
    *sigPendingPtr = Sig_Pending(Proc_GetCurrentProc());
    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_GetGroupIDsStub --
 *
 *	Return the groups that a process belongs to.
 *
 * Results:
 *	Returns KERN_SUCCESS.  Fills in the Sprite status code from 
 *	Proc_GetGroupIDs and the "pending signals" flag..  Fills in the
 *	array of group IDs with the group IDs that the current process
 *	belongs to.  Fills in the number of group IDs returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
Proc_GetGroupIDsStub(serverPort, numGIDsPtr, gidArray, statusPtr,
		     sigPendingPtr)
    mach_port_t serverPort;
    int *numGIDsPtr;		/* IN: size of gidArray; OUT: number of IDs 
				 * that the process has */
    vm_address_t gidArray;	/* array to fill in */
    ReturnStatus *statusPtr;	/* OUT: Sprite return status */
    boolean_t *sigPendingPtr;	/* OUT: is there a signal pending */
{
#ifdef lint
    serverPort = serverPort;
#endif
    *statusPtr = Proc_GetGroupIDs(*numGIDsPtr, (int *)gidArray,
				  numGIDsPtr);
    *sigPendingPtr = Sig_Pending(Proc_GetCurrentProc());
    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_GetPCBInfoStub --
 *
 *	Get information from the process control block table.  See 
 *	Proc_GetPCBInfo. 
 *
 * Results:
 *	Returns KERN_SUCCESS.  Fills in the Sprite status code from
 *	Proc_GetPCBInfo and the "pending signals" flag..  If successful,
 *	fills in (1) the PCB array with information about the requested
 *	processes and (2) the number of entries in the array that were
 *	filled in.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
Proc_GetPCBInfoStub(serverPort, firstPid, lastPid, hostID, infoSize,
		    infoArray, argStrings, statusPtr, pcbsUsedPtr,
		    sigPendingPtr)
    mach_port_t serverPort;
    Proc_PID firstPid;		/* first pid to look at, or PROC_MY_PID */
    Proc_PID lastPid;		/* last pid to look at */
    int hostID;			/* host to query, or PROC_MY_HOSTID */
    int infoSize;		/* expected size for one pcb */
    vm_address_t infoArray;	/* address of pcb array to fill in */
    vm_address_t argStrings;	/* address of "ps" arg strings to fill in */
    ReturnStatus *statusPtr;	/* OUT: Sprite result code */
    int *pcbsUsedPtr;		/* OUT: number of array elements filled in */
    boolean_t *sigPendingPtr;	/* OUT: is there a signal pending */
{
#ifdef lint
    serverPort = serverPort;
#endif
    *statusPtr = Proc_GetPCBInfo(firstPid, lastPid, hostID, infoSize,
				 (Address)infoArray,
				 (Proc_PCBArgString *)argStrings,
				 pcbsUsedPtr);
    *sigPendingPtr = Sig_Pending(Proc_GetCurrentProc());
    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_SetFamilyIDStub --
 *
 *	Change the family ID (process group) for the given process.
 *
 * Results:
 *	Returns KERN_SUCCESS.  Fills in the Sprite status code from 
 *	Proc_SetFamilyID and the "pending signals" flag.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
Proc_SetFamilyIDStub(serverPort, pid, familyID, statusPtr, sigPendingPtr)
    mach_port_t serverPort;
    Proc_PID pid;		/* the process to get the new family ID */
    Proc_PID familyID;		/* the new family ID */
    ReturnStatus *statusPtr;	/* OUT: Sprite status code */
    boolean_t *sigPendingPtr;	/* OUT: is there a signal pending */
{
#ifdef lint
    serverPort = serverPort;
#endif
    *statusPtr = Proc_SetFamilyID(pid, familyID);
    *sigPendingPtr = Sig_Pending(Proc_GetCurrentProc());
    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_SetGroupIDsStub --
 *
 *	Change the list of groups that a process belongs to.
 *
 * Results:
 *	Returns KERN_SUCCESS.  Fills in the Sprite status code from 
 *	Proc_SetGroupIDsStub and the "pending signals" flag.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
Proc_SetGroupIDsStub(serverPort, numGIDs, gidArray, statusPtr,
		     sigPendingPtr)
    mach_port_t serverPort;
    int numGIDs;		/* number of elements in gidArray */
    vm_address_t gidArray;	/* array of group IDs to use */
    ReturnStatus *statusPtr;	/* OUT: Sprite status code */
    boolean_t *sigPendingPtr;	/* OUT: is there a signal pending */
{
#ifdef lint
    serverPort = serverPort;
#endif
    *statusPtr = Proc_SetGroupIDs(numGIDs, (int *)gidArray);
    *sigPendingPtr = Sig_Pending(Proc_GetCurrentProc());
    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_SetIDsStub --
 *
 *	Set the real and effective user IDs for the current process.
 *
 * Results:
 *	Returns KERN_SUCCESS.  Fills in the Sprite status code from 
 *	Proc_SetIDs and the "pending signals" flag.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
Proc_SetIDsStub(serverPort, userID, effUserID, statusPtr, sigPendingPtr)
    mach_port_t serverPort;
    int userID;			/* real user ID */
    int effUserID;		/* effective user ID */
    ReturnStatus *statusPtr;	/* OUT: Sprite status code */
    boolean_t *sigPendingPtr;	/* OUT: is there a signal pending */
{
#ifdef lint
    serverPort = serverPort;
#endif
    *statusPtr = Proc_SetIDs(userID, effUserID);
    *sigPendingPtr = Sig_Pending(Proc_GetCurrentProc());
    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_SetIntervalTimerStub --
 *
 *	Set the current process's specified interval timer.
 *
 * Results:
 *	Returns KERN_SUCCESS.  Fills in the Sprite status code, "pending 
 *	signals" flag, and previous timer setting.
 *
 * Side effects:
 *	See Proc_SetIntervalTimer.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
Proc_SetIntervalTimerStub(serverPort, timerType, newTimer, statusPtr,
			  oldTimerPtr, sigPendingPtr)
    mach_port_t serverPort;
    int timerType;		/* which timer to set */
    Proc_TimerInterval newTimer; /* new setting for the timer */
    ReturnStatus *statusPtr;	/* OUT: Sprite status code */
    Proc_TimerInterval *oldTimerPtr; /* OUT: previous setting for the timer */
    boolean_t *sigPendingPtr;	/* OUT: is there a signal pending */
{
#ifdef lint
    serverPort = serverPort;
#endif
    *statusPtr = Proc_SetIntervalTimer(timerType, &newTimer, oldTimerPtr);
    *sigPendingPtr = Sig_Pending(Proc_GetCurrentProc());
    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_GetIntervalTimerStub --
 *
 *	Get the setting of the current process's specified interval timer.
 *
 * Results:
 *	Returns KERN_SUCCESS.  Fills in the Sprite status code, "pending 
 *	signals" flag, and timer setting.
 *
 * Side effects:
 *	See Proc_GetIntervalTimer.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
Proc_GetIntervalTimerStub(serverPort, timerType, statusPtr, timerPtr,
			  sigPendingPtr)
    mach_port_t serverPort;
    int timerType;		/* which timer to get */
    ReturnStatus *statusPtr;	/* OUT: Sprite status code */
    Proc_TimerInterval *timerPtr; /* OUT: current setting for the timer */
    boolean_t *sigPendingPtr;	/* OUT: is there a signal pending */
{
#ifdef lint
    serverPort = serverPort;
#endif
    *statusPtr = Proc_GetIntervalTimer(timerType, timerPtr);
    *sigPendingPtr = Sig_Pending(Proc_GetCurrentProc());
    return KERN_SUCCESS;
}
