/* 
 * fsInit.c --
 *
 *	Filesystem initializtion.  This is done by setting up the
 *	initial process and then having all other processes
 *	inherit things.
 *
 * Copyright (C) 1987 Regents of the University of California
 * All rights reserved.
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
#endif not lint


#include "sprite.h"

#include "fs.h"
#include "fsInt.h"
#include "fsOpTable.h"
#include "fsDevice.h"
#include "fsPrefix.h"
#include "fsLocalDomain.h"
#include "fsNameHash.h"
#include "fsRecovery.h"
#include "fsPdev.h"
#include "fsTrace.h"
#include "fsStat.h"
#include "fsClient.h"
#include "proc.h"
#include "rpc.h"
#include "recov.h"
#include "timer.h"
#include "trace.h"
/*
 * The prefix under which the local disk is attached.  
 */
#define LOCAL_DISK_NAME		"/bootTmp"
int fsDefaultDomainNumber = 0;

/*
 * We record the maximum transfer size supported by the RPC system
 * for use in chopping up remote I/O operations.
 *
 * If these are initialized to zero they will be set to the maximum
 * sizes reported back by the RPC system.
 */
int fsMaxRpcDataSize = 0;	/* Used to be 4096 */
int fsMaxRpcParamSize = 0;	/* Used to be 1024 */

/*
 * Statistics structure.
 */
FsStats	fsStats;

/*
 * Timer queue element for updating time of day.
 */

Timer_QueueElement	fsTimeOfDayElement;

/* 
 * Flag to make sure we only do certain things, such as syncing the disks,
 * after the file system has been initialized.
 */
Boolean fsInitialized = FALSE;

/*
 * Flag to indicate whether we have attached a disk or not.  We can use
 * this to determine if we are a client or a server.
 */

Boolean fsDiskAttached = FALSE;


/*
 *----------------------------------------------------------------------
 *
 * Fs_Init
 *
 *	Initialize the filesystem.  As well as initialize various
 *	tables and lists, this does the first steps in boot-strapping
 *	the name space.  The prefix table is primed with "/", and
 *	a local disk is attached under "/local" if possible.  Later
 *	in Fs_ProcInit "/local" gets promoted to root if noone
 *	else is around to serve root.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Data structures in the file system are initialized.
 *	"/" is stuck in the prefix table with no handle.
 *	Local disk is attached under "/local".
 *
 *----------------------------------------------------------------------
 */
void
Fs_Init()
{
    ReturnStatus status;
    Time time;
    int	i;
    Fs_Device defaultDisk;

    bzero((Address) &fsStats, sizeof(FsStats));
    /*
     * The handle cache and the block cache start out with a hash table of
     * a given size which grows on demand.  Thus the numbers passed to
     * the next two routines are not crucial.
     */
    FsHandleInit(64);
    FsBlockCacheInit(64);

    FsPrefixInit();

    FsClientInit();

    FsLocalDomainInit();

    FsTraceInit();
    FsPdevTraceInit();

    /*
     * Put the routine on the timeout queue that keeps the time in
     * seconds up to date.  WHY NOT USE Timer_GetTimeOfDay?
     */

    fsTimeOfDayElement.routine = FsUpdateTimeOfDay;
    fsTimeOfDayElement.clientData = 0;
    fsTimeOfDayElement.interval = timer_IntOneSecond;
    Timer_ScheduleRoutine(&fsTimeOfDayElement, TRUE);
    Timer_GetTimeOfDay(&time, (int *) NIL, (Boolean *) NIL);
    fsTimeInSeconds = time.seconds;

    /*
     * Install a crash callback with the recovery module.  (A reboot
     * callback is installed later only if we have to recover with a host.)
     */

    Recov_CrashRegister(FsClientCrashed, (ClientData) NIL);

    /*
     * This is the initial step in boot-strapping the name space.  Place
     * an entry for "/" in the prefix table.  The NIL token will cause a
     * broadcast to get a valid token the first time the prefix is used.
     */
    (void)FsPrefixInstall("/", (FsHandleHeader *)NIL, -1, FS_IMPORTED_PREFIX); 

    for (i=0 ; i<devNumDefaultDiskPartitions; i++) {
	/*
	 * Second step in bootstrapping the name space.  Try and attach
	 * a disk under the /local prefix.  Later this local partition
	 * may be promoted to the root domain (see Fs_ProcInit).
	 */
	defaultDisk = devFsDefaultDiskPartitions[i];

	status = FsAttachDisk(&defaultDisk, LOCAL_DISK_NAME, FS_ATTACH_LOCAL);
	if (status == SUCCESS) {
	    printf("Attached disk type %d unit %d to \"%s\"\n",
		     defaultDisk.type, defaultDisk.unit, LOCAL_DISK_NAME);
	    fsDiskAttached = TRUE;
	    break;
	}
    }
    fsInitialized = TRUE;
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_Bin --
 *
 *	Setup objects to be binned.
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
Fs_Bin()
{
    Mem_Bin(sizeof(FsLocalFileIOHandle));
    Mem_Bin(sizeof(FsRmtFileIOHandle));
    Mem_Bin(sizeof(FsFileDescriptor));
    Mem_Bin(sizeof(PdevServerIOHandle));
    Mem_Bin(sizeof(PdevControlIOHandle));
#ifdef INET
    FsSocketBin();
#endif
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_ProcInit
 *
 *	Initialize the filesystem part of the process table entry.
 *	This has to be called after Proc_InitMainProc
 *	because of the call to Proc_GetCurrentProc.
 *	It should be done after Rpc_Start because it might do
 *	an RPC to open the current directory.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The current directory is opened for the main process.
 *	The permissions mask is initialized.
 *	The table of open file Id's is cleared to zero size.
 *
 *----------------------------------------------------------------------
 */
void
Fs_ProcInit()
{
    ReturnStatus	status;		/* General status code return */
    Proc_ControlBlock	*procPtr;	/* Main process's proc table entry */
    register Fs_ProcessState	*fsPtr;	/* FS state ref'ed from proc table */
    char		buffer[128];
    Boolean		standalone;
    Fs_Stream		*stream;

    procPtr = Proc_GetCurrentProc();
    procPtr->fsPtr = fsPtr = mnew(Fs_ProcessState);
    /*
     * General filesystem initialization.
     * Find out how much we can transfer with the RPC system.
     * (If these are already set we don't change them.)
     *
     * FIX THIS TO CALL DOMAIN SPECIFIC INITIALIZATION ROUTINES.
     */
    if (fsMaxRpcDataSize + fsMaxRpcParamSize == 0) {
	Rpc_MaxSizes(&fsMaxRpcDataSize, &fsMaxRpcParamSize);
    }

    fsPtr->numGroupIDs	= 1;
    fsPtr->groupIDs 	= (int *) malloc(1 * sizeof(int));
    fsPtr->groupIDs[0]	= 0;

    /*
     * If there is an attached disk and we are booting standalone (no -c
     * option and there is a boot directory on the attached disk) then
     * install a "/" prefix.
     */
    standalone = FALSE;
    if (fsDiskAttached) {
	int	dummy;
	int	argc;
	char	*argv[10];
	char	argBuffer[100];
	int	i;

	standalone = TRUE;
	sprintf(buffer, "%s/boot", LOCAL_DISK_NAME);
	printf("Trying to open %s.\n", buffer);
	status = Fs_Open(buffer, FS_READ, FS_DIRECTORY, 0, &dummy);
	if (status != SUCCESS) {
	    printf("Can't open %s <0x%x>\n", buffer, status);
	    standalone = FALSE;
	} else {
	    printf("Open succeeded.\n");
	    argc = Mach_GetBootArgs(10, 100, argv, argBuffer);
	    for (i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "-c")) {
		    standalone = FALSE;
		    break;
		}
	    }
	}
	if (standalone) {
	    Fs_Stream		*streamPtr;
	    static char		rootPrefix[] = "/";

	    status = Fs_Open(LOCAL_DISK_NAME, FS_READ|FS_FOLLOW,
			    FS_DIRECTORY, 0, &streamPtr);
	    if (status != SUCCESS) {
		panic("Fs_ProcInit: Unable to open local disk prefix!");
	    } else {
		printf("Installing %s prefix.\n", rootPrefix);
		FsPrefixInstall(rootPrefix, streamPtr->ioHandlePtr, 
			FS_LOCAL_DOMAIN, 
			FS_LOCAL_PREFIX|FS_IMPORTED_PREFIX|FS_OVERRIDE_PREFIX);
	    }
	}
    }
    /*
     * Try and open /.
     */
    status = FAILURE;
    printf("Trying to open /.\n");
    do {
	status = Fs_Open("/", FS_READ, FS_DIRECTORY, 0, &stream);
	if (status != SUCCESS) {
	    if (standalone) {
		panic(
		"Fs_ProcInit: Booting standalone and can't open \"/\".\n");
	    }
	    /*
	     *  Wait a bit and retry the open of "/".
	     */
	    printf(
		    "Can't find server for \"/\", waiting 1 min.\n");
	    (void)Sync_WaitTime(time_OneMinute);
	}
    } while (status != SUCCESS);
    printf("Open succeeded.\n");
    Fs_Close(stream);
    fsPtr->cwdPtr = (Fs_Stream *)NIL;
    printf("Trying to open /boot.\n");
    status = Fs_Open("/boot", FS_READ, FS_DIRECTORY, 0, &fsPtr->cwdPtr);
    if (status != SUCCESS) {
	panic("Fs_ProcInit: can't open /boot.\n");
    }
    printf("Open succeeded.\n");
    /*
     * Set the default permissions mask; it indicates the maximal set of
     * permissions that a newly created file can have.
     */
    fsPtr->filePermissions = FS_OWNER_WRITE |
			FS_OWNER_READ | FS_OWNER_EXEC |
			FS_GROUP_READ | FS_GROUP_EXEC |
			FS_WORLD_READ | FS_WORLD_EXEC;
    /*
     * The open file list is only needed by user processes.
     * Fs_GetNewID will create and grow this list.
     */
    fsPtr->numStreams = 0;
    fsPtr->streamList = (Fs_Stream **)NIL;

    return;
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_InheritState -
 *
 *	This is called during process creation to have the new process
 *	inherit filesystem state from its parent.  This includes the
 *	current directory, open files, and the permission mask.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Duplicate the filesystem state of the parent process in
 *	the new child process.
 *
 *----------------------------------------------------------------------
 */
void
Fs_InheritState(parentProcPtr, newProcPtr)
    Proc_ControlBlock *parentProcPtr;	/* Process to inherit from */
    Proc_ControlBlock *newProcPtr;	/* Process that inherits */
{
    register 	Fs_ProcessState	*parFsPtr;
    register 	Fs_ProcessState	*newFsPtr;
    register	Fs_Stream	**parStreamPtrPtr;
    register	Fs_Stream	**newStreamPtrPtr;
    register 	int 		i;
    int 			len;

    /*
     * User Ids.  This is kept separate from the file system state
     * so other modules can use the user ID for permission checking.
     */
    newProcPtr->userID = parentProcPtr->userID;

    /*
     * The rest of the filesystem state hangs off the Fs_ProcessState record.
     * This could be shared after forking to implement file descriptor sharing.
     */
    parFsPtr = parentProcPtr->fsPtr;
    newProcPtr->fsPtr = newFsPtr = mnew(Fs_ProcessState);

    /*
     * Current working directory.
     */
    if (parFsPtr->cwdPtr != (Fs_Stream *)NIL) {
	Fs_StreamCopy(parFsPtr->cwdPtr, &newFsPtr->cwdPtr);
    } else {
	newFsPtr->cwdPtr = (Fs_Stream *)NIL;
    }

    /*
     * Open stream list.
     */
    len = newFsPtr->numStreams = parFsPtr->numStreams;
    if (len > 0) {
	newFsPtr->streamList =
		(Fs_Stream **)malloc(len * sizeof(Fs_Stream *));
	newFsPtr->streamFlags = (char *)malloc(len * sizeof(char));
	for (i = 0, parStreamPtrPtr = parFsPtr->streamList,
		    newStreamPtrPtr = newFsPtr->streamList;
	     i < len;
	     i++, parStreamPtrPtr++, newStreamPtrPtr++) {
	    if (*parStreamPtrPtr != (Fs_Stream *) NIL) {
		Fs_StreamCopy(*parStreamPtrPtr, newStreamPtrPtr);
		newFsPtr->streamFlags[i] = parFsPtr->streamFlags[i];
	    } else {
		*newStreamPtrPtr = (Fs_Stream *) NIL;
		newFsPtr->streamFlags[i] = 0;
	    }
	}
    } else {
	newFsPtr->streamList = (Fs_Stream **)NIL;
	newFsPtr->streamFlags = (char *)NIL;
    }

    newFsPtr->filePermissions = parFsPtr->filePermissions;

    /*
     * Group ID list.
     */

    len = newFsPtr->numGroupIDs = parFsPtr->numGroupIDs;
    if (len) {
	newFsPtr->groupIDs = (int *)malloc(len * sizeof(int));
	for (i=0 ; i<len; i++) {
	    newFsPtr->groupIDs[i] = parFsPtr->groupIDs[i];
	}
    } else {
	newFsPtr->groupIDs = (int *)NIL;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_CloseOnExec -
 *
 *	Called just before a process overlays itself with another program.
 *	At this point any stream marked as CLOSE_ON_EXEC are closed.
 *	This makes cleanup for shell-like programs easier.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Close streams marked CLOSE_ON_EXEC.
 *
 *----------------------------------------------------------------------
 */
void
Fs_CloseOnExec(procPtr)
    Proc_ControlBlock *procPtr;		/* Process to operate on */
{
    register int i;
    register Fs_Stream **streamPtrPtr;	/* Pointer into streamList */
    char *flagPtr;			/* Pointer into flagList */

    for (i=0, streamPtrPtr = procPtr->fsPtr->streamList,
	      flagPtr = procPtr->fsPtr->streamFlags ;
	 i < procPtr->fsPtr->numStreams ;
	 i++, streamPtrPtr++, flagPtr++) {
	if ((*streamPtrPtr != (Fs_Stream *)NIL) &&
	    (*flagPtr & FS_CLOSE_ON_EXEC)) {
	    (void)Fs_Close(*streamPtrPtr);
	    *streamPtrPtr = (Fs_Stream *)NIL;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_CloseState
 *
 *	This is called when a process dies to clean up its filesystem state
 *	by closing its open files and its current directory.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All its open streams are closed, and associated memory is free'd.
 *
 *----------------------------------------------------------------------
 */
void
Fs_CloseState(procPtr)
    Proc_ControlBlock *procPtr;		/* An exiting process to clean up */
{
    register int i;
    register Fs_ProcessState *fsPtr = procPtr->fsPtr;

    if (fsPtr->cwdPtr != (Fs_Stream *) NIL) {
	(void)Fs_Close(fsPtr->cwdPtr);
    }

    if (fsPtr->streamList != (Fs_Stream **)NIL) {
	for (i=0 ; i < fsPtr->numStreams ; i++) {
	    register Fs_Stream *streamPtr;

	    streamPtr = fsPtr->streamList[i];
	    if (streamPtr != (Fs_Stream *)NIL) {
		(void)Fs_Close(streamPtr);
	    }
	}
	free((Address) fsPtr->streamList);
	free((Address) fsPtr->streamFlags);
	fsPtr->streamList = (Fs_Stream **)NIL;
    }

    if (fsPtr->groupIDs != (int *) NIL) {
	free((Address) fsPtr->groupIDs);
	fsPtr->groupIDs = (int *) NIL;
	fsPtr->numGroupIDs = 0;
    }
    free((Address)fsPtr);
    procPtr->fsPtr = (Fs_ProcessState *)NIL;
}
