/* 
 * sysCalls.c --
 *
 *	Miscellaneous system calls that are lumped under the Sys_ prefix.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "mach.h"
#include "sys.h"
#include "sysStats.h"
#include "time.h"
#include "timer.h"
#include "vm.h"
#include "machMon.h"
#include "proc.h"
#include "dbg.h"
#include "fs.h"
#include "proc.h"
#include "rpc.h"
#include "net.h"
#include "sched.h"
#include "dev.h"
#include "procMigrate.h"
#include "string.h"

Boolean	sys_ErrorShutdown = FALSE;
Boolean	sys_ShuttingDown = FALSE;
extern	Boolean	sysPanicing;


/*
 *----------------------------------------------------------------------
 *
 * Sys_GetTimeOfDay --
 *
 *	Returns the current system time to a local user process.
 *	If any argument is USER_NIL, that value is not returned.
 *
 *	The "real" time of day is returned, rather than the software
 *	time.
 *
 * Results:
 *	SUCCESS 		The call was successful.
 *	SYS_ARG_NOACCESS	The user arguments were not accessible.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Sys_GetTimeOfDay(timePtr, localOffsetPtr, DSTPtr)
    Time	*timePtr;		/* Buffer to store the UT time. */
    int		*localOffsetPtr;	/* Buffer to store the number of minutes
					 * from UT (a negative value means you 
					 * are west of the Prime Meridian. */
    Boolean	*DSTPtr;		/* Buffer to store a flag that's TRUE
					 * if DST is followed. */
{
    Time	curTime;
    int		curLocalOffset;
    Boolean	curDST;

    Timer_GetRealTimeOfDay(&curTime, &curLocalOffset, &curDST);

    if (timePtr != USER_NIL) {
	if (Proc_ByteCopy(FALSE, sizeof(Time),
	      (Address) &curTime, (Address) timePtr) != SUCCESS) {
	    return(SYS_ARG_NOACCESS);
	}
    }
    if (localOffsetPtr != USER_NIL) {
	if (Proc_ByteCopy(FALSE, sizeof(int),(Address) &curLocalOffset, 
				(Address) localOffsetPtr) != SUCCESS) {
	    return(SYS_ARG_NOACCESS);
	}
    }
    if (DSTPtr != USER_NIL) {
	if (Proc_ByteCopy(FALSE, sizeof(Boolean),(Address) &curDST,
					(Address) DSTPtr) != SUCCESS) {
	    return(SYS_ARG_NOACCESS);
	}
    }
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * Sys_SetTimeOfDay --
 *
 *	Changes the current system time to the value specified by
 *	the arguments. The timePtr argument  must be valid.
 *
 * Results:
 *	SUCCESS 		The call was successful.
 *	SYS_ARG_NOACCESS	The user arguments were not accessible.
 *
 * Side effects:
 *	The system time is updated to a new value.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Sys_SetTimeOfDay(timePtr, localOffset, DST)
    Time	*timePtr;	/* New value for the UT (GMT) time. */
    int		localOffset;	/* new value for the offset in minutes 
				 * from UT.*/
    Boolean	DST;		/* If TRUE, DST is used at this site. */
{
    Time	newTime;

    if (timePtr == USER_NIL || 
        (Proc_ByteCopy(TRUE, sizeof(Time), (Address) timePtr, 
			(Address) &newTime) != SUCCESS)) {
	return(SYS_ARG_NOACCESS);
    }
    Timer_SetTimeOfDay(newTime, localOffset, DST);
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Sys_DoNothing --
 *
 *	This system call simply returns SUCCESS. It does not perform any
 *	function.
 *
 * Results:
 *	SUCCESS 		This value is always returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Sys_DoNothing()
{
    Sched_ContextSwitch(PROC_READY);
    return(SUCCESS);
}


#define	MAX_WAIT_INTERVALS	3
Boolean	shutdownDebug = FALSE;

/*
 *----------------------------------------------------------------------
 *
 * Sys_Shutdown --
 *
 *	This system call calls appropriate routines to shutdown
 *	the system in an orderly fashion.
 *
 * Results:
 *	SUCCESS 	This value is always returned.
 *	SYS_ARG_NOACESS	The reboot string was inaccessible.	
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Sys_Shutdown(flags, rebootString)
    int		flags;
    char	*rebootString;
{
    Time		waitTime;
    int			alive;
    int			timesWaited;
    Boolean		userDead = FALSE;
    char		string[100];
    int			accLength;
    int			strLength;
    ReturnStatus	status;
    Proc_ControlBlock	*procPtr;


    if (flags & SYS_REBOOT) {
	if (rebootString == (char *) USER_NIL) {
	    string[0] = '\0';
	} else {
	    status = Proc_MakeStringAccessible(100, &rebootString, &accLength,
						    &strLength);
	    if (status != SUCCESS) {
		return(SYS_ARG_NOACCESS);
	    }
	    (void) String_Copy(rebootString, string);
	    Proc_MakeUnaccessible(rebootString, accLength);
	}
    }

    if (flags & SYS_DEBUG) {
	sys_ErrorShutdown = TRUE;
    }

    if (flags & SYS_KILL_PROCESSES) {
	/*
	 * Turn ourselves into a kernel process since we no longer need
	 * user process resources.
	 */
	procPtr = Proc_GetCurrentProc();
	Proc_Lock(procPtr);
	procPtr->genFlags &= ~PROC_USER;
	procPtr->genFlags |= PROC_KERNEL;
	Proc_Unlock(procPtr);
	VmMach_ReinitContext(procPtr);

	waitTime.seconds = 5;
	waitTime.microseconds = 0;
	while (TRUE) {
	    if (userDead) {
		sys_ShuttingDown = TRUE;
	    }
	    timesWaited = 0;
	    while (TRUE) {
		alive = Proc_KillAllProcesses(!userDead);
		if (alive == 0) {
		    break;
		}
		if (timesWaited >= MAX_WAIT_INTERVALS) {
		    Sys_Printf("%d %s processes still alive.\n", alive,
				userDead ? "kernel" : "user");
		    break;
		}
		timesWaited++;
		Sys_Printf("Waiting with %d %s processes still alive\n", alive,
				userDead ? "kernel" : "user");
		if (shutdownDebug) {
		    DBG_CALL;
		}
		(void) Sync_WaitTime(waitTime);
	    }
	    if (userDead) {
		break;
	    }
	    userDead = TRUE;
	}
	/*
	 * Give this process highest priority so that no other process 
	 * can interrupt it.
	 */
	(void) Proc_SetPriority(PROC_MY_PID, PROC_NO_INTR_PRIORITY, FALSE);
    }

    /*
     * Sync the disks.
     */
    if (flags & SYS_WRITE_BACK) {
	Sys_Printf("Syncing disks\n");
	Fs_Sync(-1, flags & SYS_KILL_PROCESSES);
    }

    if (flags & SYS_HALT) {
	Mach_MonAbort();
    } else if (flags & SYS_REBOOT) {
	Mach_MonReboot(string);
    } else if (flags & SYS_DEBUG) {
	sys_ShuttingDown = FALSE;
	sys_ErrorShutdown = FALSE;
	DBG_CALL;
    }

    return(SUCCESS);
}

Boolean	sys_ShouldSyncDisks = TRUE;

/*
 *----------------------------------------------------------------------
 *
 * SysErrorShutdown --
 *
 *	This routine is called when the system encountered an error and
 *	needs to be shut down.
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
SysErrorShutdown(trapType)
    int		trapType;
{
    if (sys_ShouldSyncDisks && !mach_AtInterruptLevel && !sys_ShuttingDown &&
        !dbg_BeingDebugged && (trapType != MACH_BRKPT_TRAP || sysPanicing)) {
	sys_ErrorShutdown = TRUE;
	(void) Sys_Shutdown(SYS_KILL_PROCESSES);
    }
    if (sys_ShouldSyncDisks && sys_ShuttingDown) {
	Sys_Printf("Error type %d while shutting down system. Exiting ...\n",
		   trapType);
	Proc_Exit(trapType);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Sys_SyncDisks --
 *
 *	This routine is called when the system encountered an error and
 *	the disks should be synced.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */


static	Boolean	errorSync = FALSE;


void
Sys_SyncDisks(trapType)
    int	trapType;
{
    char	*SpriteVersion();
    
    if (errorSync) {
	Sys_Printf("Error type %d while syncing disks.\n", trapType);
	sys_ShouldSyncDisks = FALSE;
	errorSync = FALSE;
	DBG_CALL;
	return;
    }
    if (sys_ShouldSyncDisks && !mach_AtInterruptLevel && !sys_ShuttingDown &&
        !dbg_BeingDebugged && (trapType != MACH_BRKPT_TRAP || sysPanicing)) {
	Sys_Printf("Syncing disks.  Version: %s\n", SpriteVersion());
	errorSync = TRUE;
	Fs_Sync(-1, TRUE);
	errorSync = FALSE;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Sys_GetMachineInfo --
 *
 *	Returns the machine architecture and type information.
 *
 * Results:
 *	SUCCESS 		The call was successful.
 *	SYS_ARG_NOACCESS	The user arguments were not accessible.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Sys_GetMachineInfo(archPtr, typePtr, clientIDPtr)
    int	*archPtr;	/* Buffer to hold machine architecture #. */
    int	*typePtr;	/* Buffer to hold machine type. */
    int	*clientIDPtr;	/* Buffer to hold client ID. */
{

    if (archPtr != (int *) USER_NIL) {
	int arch = Mach_GetMachineArch();

	if (Proc_ByteCopy(FALSE, sizeof(int), (Address) &arch, 
				(Address) archPtr) != SUCCESS) {
	    return(SYS_ARG_NOACCESS);
	}
    }

    if (typePtr != (int *) USER_NIL) {

	int type = Mach_GetMachineType();

	if (Proc_ByteCopy(FALSE, sizeof(int), (Address) &type, 
				(Address) typePtr) != SUCCESS) {
	    return(SYS_ARG_NOACCESS);
	}
    }


    if (clientIDPtr != (int *) USER_NIL) {
	if (Proc_ByteCopy(FALSE, sizeof(int), (Address) &rpc_SpriteID, 
				(Address) clientIDPtr) != SUCCESS) {
	    return(SYS_ARG_NOACCESS);
	}
    }

    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Sys_StatsStub --
 *
 *	System call stub for the Statistics hook.
 *
 * Results:
 *	SUCCESS			- the data were returned.
 *	GEN_INVALID_ARG		- if a bad argument was passed in.
 *	?			- result from Vm_CopyOut.
 *	
 *
 * Side effects:
 *	Fill in the requested statistics.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Sys_StatsStub(command, option, argPtr)
    int command;		/* Specifies what to do */
    int option;			/* Modifier for command */
    Address argPtr;		/* Argument for command */
{
    ReturnStatus status = SUCCESS;
    
    switch(command) {
	case SYS_GET_VERSION_STRING: {
	    /*
	     * option is the length of the storage referenced by argPtr.
	     */
	    register int length;
	    register char *version;
	    version = (char *)SpriteVersion();
	    length = String_Length(version) + 1;
	    if (option <= 0) {
		status = GEN_INVALID_ARG;
		break;
	    }
	    if (option < length) {
		length = option;
	    }
	    status = Vm_CopyOut(length, version, argPtr);
	    break;
	}
	case SYS_SYNC_STATS: {
	    register Sync_Instrument *syncStatPtr;

	    syncStatPtr = (Sync_Instrument *)argPtr;
	    if (syncStatPtr == (Sync_Instrument *)NIL ||
		syncStatPtr == (Sync_Instrument *)0 ||
		syncStatPtr == (Sync_Instrument *)USER_NIL) {
		
		Sync_PrintStat();
	    } else {
		status = Vm_CopyOut(sizeof(Sync_Instrument),
				  (Address)&sync_Instrument,
				  (Address) syncStatPtr);
	    }
	    break;
	}
	case SYS_VM_STATS: {
	    if (argPtr == (Address)NIL ||
		argPtr == (Address)0 ||
		argPtr == (Address)USER_NIL) {
		return(GEN_INVALID_ARG);
	    } else {
		status = Vm_CopyOut(sizeof(Vm_Stat), (Address)&vmStat, argPtr);
	    }
	    break;
	}
	case SYS_SCHED_STATS: {
	    Sched_Instrument *schedStatPtr;
	    Time curTime;

	    schedStatPtr = (Sched_Instrument *)argPtr;
	    if (schedStatPtr == (Sched_Instrument *)NIL ||
		schedStatPtr == (Sched_Instrument *)0 ||
		schedStatPtr == (Sched_Instrument *)USER_NIL) {
		
		Sched_PrintStat();
	    } else {
		Timer_TicksToTime(sched_Instrument.noProcessRunning,
				  &sched_Instrument.idleTime);
		/*
		 * If no interrupts received, mostRecentInterrupt will
		 * be 0, so set it to the current time.  This will happen
		 * the first time Sched_Stats is called if there were no
		 * keyboard interrupts already.
		 *
		 * Note: mostRecentInterrupt can't be set during dev_Kbd
		 * initialization because the timer has not yet been
		 * initialized.
		 */
		if (dev_KbdInstrument.mostRecentInterrupt.seconds == 0) {
		    Timer_GetTimeOfDay(&dev_KbdInstrument.mostRecentInterrupt,
				       (int *) NIL, (Boolean *) NIL);
		}
		Timer_GetTimeOfDay(&curTime, (int *) NIL, (Boolean *) NIL);
		Time_Subtract(curTime, dev_KbdInstrument.mostRecentInterrupt,
			      &sched_Instrument.noUserInput);
		status = Vm_CopyOut(sizeof(Sched_Instrument),
					  (Address)&sched_Instrument, argPtr);
	    }
	    break;
	}
	case SYS_RPC_CLT_STATS:
	case SYS_RPC_SRV_STATS:
	case SYS_RPC_TRACE_STATS:
	case SYS_RPC_SERVER_HIST:
	case SYS_RPC_CLIENT_HIST:
	case SYS_RPC_SRV_STATE:
	case SYS_RPC_CLT_STATE:
	case SYS_RPC_ENABLE_SERVICE:
	    status = Rpc_GetStats(command, option, argPtr);
	    break;

	case SYS_PROC_MIGRATION: {
	    switch(option) {
		case SYS_PROC_MIG_ALLOW: 
		case SYS_PROC_MIG_REFUSE: {
		    register Proc_ControlBlock *procPtr;
		    procPtr = Proc_GetEffectiveProc();
		    if (procPtr->effectiveUserID != 0) {
			status = GEN_NO_PERMISSION;
		    } else {
			proc_RefuseMigrations =
				(option == SYS_PROC_MIG_REFUSE);
		    }
		}
		break;

	        case SYS_PROC_MIG_GET_STATUS: {
		    if (argPtr != (Address) NIL) {
			status = Vm_CopyOut(sizeof(Boolean),
					    (Address)&proc_RefuseMigrations,
					    argPtr);
		    } else {
			status = GEN_INVALID_ARG;
		    }
		}
		break;
		case SYS_PROC_MIG_SET_DEBUG: {
		    int arg;
		    status = Vm_CopyIn(sizeof(int), (Address)&arg, argPtr);
		    if (status == SUCCESS && arg >= 0) {
			proc_MigDebugLevel = option;
		    } else if (status == SUCCESS) {
			status = GEN_INVALID_ARG;
		    }
		}
		break;

		default:{
		    status = GEN_INVALID_ARG;
		}
		break;
	    }
	    break;
	}
	
	case SYS_PROC_TRACE_STATS: {
	    switch(option) {
		case SYS_PROC_TRACING_PRINT:
		    Sys_Panic(SYS_WARNING,
		      "Printing of proc trace records not implemented.\n");
		    break;
		case SYS_PROC_TRACING_ON:
		    Proc_MigrateStartTracing();
		    break;
		case SYS_PROC_TRACING_OFF:
		    proc_DoTrace = FALSE;
		    break;
		default:
		    /*
		     * The default is to copy 'option' trace records.
		     */
		    status = Trace_Dump(proc_TraceHdrPtr, option, argPtr);
		    break;
	    }
	    break;
	}
	case SYS_FS_PREFIX_STATS: {
	    status = Fs_PrefixDump(option, argPtr);
	    break;
	}
	case SYS_FS_PREFIX_EXPORT: {
	    status = Fs_PrefixDumpExport(option, argPtr);
	    break;
	}
	case SYS_SYS_CALL_STATS: {
	    status = Sys_OutputNumCalls(option, argPtr);
	    break;
	}
	case SYS_NET_GET_ROUTE: {
	    status = Net_IDToRouteStub(option, argPtr);
	    break;
	}
	case SYS_NET_ETHER_STATS:
	    status = Vm_CopyOut(sizeof(Net_EtherStats),
				(Address)&net_EtherStats, (Address)argPtr);
	    break;
	case SYS_DISK_STATS: {
	    int			bytesAcc;
	    Sys_DiskStats	*statArrPtr;

	    Vm_MakeAccessible(VM_OVERWRITE_ACCESS,
			      sizeof(Sys_DiskStats) * option,
			      (Address)argPtr, &bytesAcc, &statArrPtr);
	    if (statArrPtr == (Sys_DiskStats *)NIL) {
		status = SYS_ARG_NOACCESS;
	    } else {
		Dev_GetDiskStats(statArrPtr, bytesAcc / sizeof(Sys_DiskStats));
		Vm_MakeUnaccessible((Address)statArrPtr, bytesAcc);
		status = SUCCESS;
	    }
	    break;
	}
	default:
	    status = GEN_INVALID_ARG;
	    break;
    }
    return(status);
}

