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
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/sprited/fs/RCS/fsInit.c,v 1.3 92/03/12 17:40:38 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint


#include <sprite.h>
#include <ckalloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fs.h>
#include <fsutil.h>
#include <fsNameOps.h>
#include <fsio.h>
#include <fsprefix.h>
#include <fslcl.h>
#ifdef SPRITED_LOCALDISK
#include <devFsOpTable.h>
#endif
#include <fspdev.h>
#include <fsutilTrace.h>
#include <fsStat.h>
#include <fsconsist.h>
#include <proc.h>
#include <rpc.h>
#include <recov.h>
#include <timer.h>
#include <trace.h>
#include <fsdm.h>
#include <fsrmt.h>
#include <devTypes.h>


#define	SCSI_MAKE_DEVICE_TYPE(type, hbaType, ctrlNum, targetID, LUN, dBits) \
		(((hbaType)<<8)|(type))
#define	SCSI_MAKE_DEVICE_UNIT(type, hbaType, ctrlNum, targetID, LUN, dBits)  \
		(((ctrlNum)<<10)|((LUN)<<7)|((targetID)<<4)|(dBits))

/*
 * The prefix under which the local disk is attached.  
 */
#define LOCAL_DISK_NAME		"/"
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
Fs_Stats	fs_Stats;

/*
 * Timer queue element for updating time of day.
 */

Timer_QueueElement	fsutil_TimeOfDayElement;

/* 
 * Flag to make sure we only do certain things, such as syncing the disks,
 * after the file system has been initialized.
 */
Boolean fsutil_Initialized = FALSE;

/*
 * Flag to indicate whether we have attached a disk or not.  We can use
 * this to determine if we are a client or a server.
 */

Boolean fsDiskAttached = FALSE;


void Fs_Init()
{
    Fs_InitData();
    Fs_InitNameSpace();
}
/*
 *----------------------------------------------------------------------
 *
 * Fs_InitData
 *
 *	Initialize most filesystem data structures.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Data structures in the file system are initialized.
 *
 *----------------------------------------------------------------------
 */
void
Fs_InitData()
{
    /* 
     * Verify that the MIG definition of various types agrees with the C 
     * definition.
     */
    if (sizeof(Fs_Attributes) != FS_ATTRIBUTES_SIZE * sizeof(int)) {
	panic("Fs_InitData: size mismatch for Fs_Attributes.\n");
    }
    if (sizeof(Fs_Device) != FS_DEVICE_SIZE * sizeof(int)) {
	panic("Fs_InitData: size mismatch for Fs_Device.\n");
    }

    /*
     * Initialized the known domains. Local, Remote, and Pfs.
     */
#ifdef SPRITED_LOCALDISK
    Fslcl_NameInitializeOps();
#endif
    Fsio_InitializeOps();
    Fsrmt_InitializeOps();
    Fspdev_InitializeOps();

    bzero((Address) &fs_Stats, sizeof(Fs_Stats));
    fs_Stats.statsVersion = FS_STAT_VERSION;

    /*
     * The handle cache and the block cache start out with a hash table of
     * a given size which grows on demand.  Thus the numbers passed to
     * the next two routines are not crucial.
     */
    Fscache_Init(64);
    Fsutil_HandleInit(64);

    Fsprefix_Init();

    Fsconsist_ClientInit();

#ifdef SPRITED_LOCALDISK
    Fslcl_DomainInit();
#endif

    Fsutil_TraceInit();
    Fspdev_TraceInit();
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_InitNameSpace
 *
 *	Initialize the filesystem name space.
 *	This does the first steps in boot-strapping
 *	the name space.  The prefix table is primed with "/", and
 *	a local disk is attached under "/bootTmp" if possible.  Later
 *	in Fs_ProcInit "/bootTmp" gets promoted to root if noone
 *	else is around to serve root.
 *
 *	This also initializes the file system's time (which could
 *	perhaps be replaced someday).  This has to be done after
 *	Rpc_Start.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	"/" is stuck in the prefix table with no handle.
 *	Local disk is attached under "/bootTmp".
 *
 *----------------------------------------------------------------------
 */
void
Fs_InitNameSpace()
{

    /*
     * Install a crash callback with the recovery module.  (A reboot
     * callback is installed later only if we have to recover with a host.)
     */

    Recov_CrashRegister(Fsutil_ClientCrashed, (ClientData) NIL);

    /*
     * This is the initial step in boot-strapping the name space.  Place
     * an entry for "/" in the prefix table.  The NIL token will cause a
     * broadcast to get a valid token the first time the prefix is used.
     */
    (void)Fsprefix_Install("/", (Fs_HandleHeader *)NIL, -1, FSPREFIX_IMPORTED); 

    fsutil_Initialized = TRUE;
}


#ifdef SPRITED_NATIVE_MALLOC
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
    Fsio_Bin();
    Fspdev_Bin();
    Fsrmt_Bin();
#ifdef INET
    FsSocketBin();
#endif
    Mem_Bin(FS_BLOCK_SIZE);
}
#endif /* SPRITED_NATIVE_MALLOC */


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
    Fs_Stream		*stream;
    Time		waitTime;
    Time		incrTime;
#ifdef SPRITED_LOCALDISK
    Fs_Device 		*defaultDiskPtr;
    Boolean		stdDefaults;
    int			numDefaults;
    int			argc;
    char		*argv[10];
    int			i;
    char		argBuffer[256];
#endif

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
    fsPtr->groupIDs 	= (int *) ckalloc(1 * sizeof(int));
    fsPtr->groupIDs[0]	= 0;

#ifdef SPRITED_LOCALDISK
    defaultDiskPtr = (Fs_Device *) ckalloc(sizeof(Fs_Device));
    numDefaults = devNumDefaultDiskPartitions;
    stdDefaults = TRUE;
    argc = Mach_GetBootArgs(10, 256, argv, argBuffer);
    for (i = 0; i < argc; i++) {
	if (!strcasecmp(argv[i], "-rootdisk")) {
	    if (argc == i + 1) {
		printf("-rootdisk option requires an argument\n");
	    } else {
		int	type;
		int	unit;
		int	n;
		n = sscanf(argv[i+1], " %d.%d ", &type, &unit);
		if (n != 2) {
		    printf("-rootdisk has the following syntax:\n");
		    printf("\t-rootdisk <type>.<unit>\n");
		} else {
		    printf("Found -rootdisk option, %d.%d\n",
			type, unit);
		    defaultDiskPtr->serverID = -1;
		    defaultDiskPtr->type = type;
		    defaultDiskPtr->unit = unit;
		    defaultDiskPtr->data = (ClientData) NIL;
		    numDefaults = 1;
		    stdDefaults = FALSE;
		}
	    }
	}
    }
    for (i=0 ; i< numDefaults ; i++) {
	/*
	 * Second step in bootstrapping the name space.  Try and attach
	 * a disk. 
	 */
	if (stdDefaults) {
	    *defaultDiskPtr = devFsDefaultDiskPartitions[i];
	}

	status = Fsdm_AttachDisk(defaultDiskPtr, LOCAL_DISK_NAME, 
		    FS_ATTACH_LOCAL | FS_DEFAULT_DOMAIN);
	if (status == SUCCESS) {
	    Fs_Attributes	attr;
	    int			i;
	    char		buffer[128];
	    Boolean		rootServer = FALSE;

	    sprintf(buffer, "%s/ROOT", LOCAL_DISK_NAME);
	    status = Fs_GetAttributes(buffer, FS_ATTRIB_FILE, &attr);
	    if (status != SUCCESS) {
		printf("Stat of %s returned 0x%x\n", buffer, status);
		rootServer = FALSE;
	    } else {
		printf("Found %s.\n", buffer);
		rootServer = TRUE;
	    }
	    argc = Mach_GetBootArgs(10, 256, argv, argBuffer);
	    for (i = 0; i < argc; i++) {
		if (!strcasecmp(argv[i], "-backup")) {
		    printf("Found %s option.\n", argv[i]);
		    rootServer = FALSE;
		}
		if (!strcasecmp(argv[i], "-root")) {
		    printf("Found %s option.\n", argv[i]);
		    rootServer = TRUE;
		}
	    }
#ifdef SPRITED
	    /* 
	     * Once we think the file system (including local disk access)
	     * is working, we can take this out.
	     */
	    panic("System thinks it's the root server.\n");
#endif
	    if (rootServer) {
		sprintf(buffer, "%s/boot", LOCAL_DISK_NAME);
		status = Fs_GetAttributes(buffer, FS_ATTRIB_FILE, &attr);
		if (status != SUCCESS) {
		    printf("/boot not found on root partition.\n");
		    rootServer = FALSE;
		} else {
		    printf("Found /boot.\n");
		    if ((attr.type != FS_DIRECTORY)) {
			printf("/boot is not a directory!\n");
			rootServer = FALSE;
		    } else {
			printf("/boot is a directory.\n");
		    }
		}
	    }
	    if (rootServer) {
		Fs_Stream		*streamPtr;
		static char		rootPrefix[] = "/";
    
		status = Fs_Open(LOCAL_DISK_NAME, FS_READ|FS_FOLLOW,
				FS_DIRECTORY, 0, &streamPtr);
		if (status != SUCCESS) {
		    printf("Fs_ProcInit: Unable to open local disk prefix!");
		} else {
		    printf("Installing the local disk as %s.\n", rootPrefix);
		    (void)Fsprefix_Install(rootPrefix, streamPtr->ioHandlePtr, 
			    FS_LOCAL_DOMAIN, 
			    FSPREFIX_LOCAL|FSPREFIX_IMPORTED|FSPREFIX_OVERRIDE);
		    if (strcmp(LOCAL_DISK_NAME, rootPrefix)) {
			printf("Clearing local disk prefix %s.\n", 
			    LOCAL_DISK_NAME);
			Fsprefix_Clear(LOCAL_DISK_NAME, TRUE, FALSE);
		    }
		    fsDiskAttached = TRUE;
		    break;
		}
	    } else {
		printf("Detaching the local disk.\n");
		Fsdm_DetachDisk(LOCAL_DISK_NAME);
	    }
	}
    }
#endif /* SPRITED_LOCALDISK */
    /*
     * Try and open /.
     */
    status = FAILURE;
    Time_Multiply(time_OneSecond, 5, &incrTime);
    waitTime = time_ZeroSeconds;
    do {
	if (Time_LT(waitTime, time_OneMinute)) {
	    Time_Add(waitTime, incrTime, &waitTime);
	}
	status = Fs_Open("/", FS_READ, FS_DIRECTORY, 0, &stream);
	if (status != SUCCESS) {
	    if (fsDiskAttached) {
		panic(
		"Fs_ProcInit: I'm the root server and I can't open \"/\".\n");
	    }
	    /*
	     *  Wait a bit and retry the open of "/".
	     */
	    printf("Can't find server for \"/\", waiting %d seconds.\n",
		waitTime.seconds);
	    (void)Sync_WaitTime(waitTime);
	}
    } while (status != SUCCESS);
    Fs_Close(stream);
    fsPtr->cwdPtr = (Fs_Stream *)NIL;
    status = Fs_Open("/boot", FS_READ, FS_DIRECTORY, 0, &fsPtr->cwdPtr);
    if (status != SUCCESS) {
	panic("Fs_ProcInit: can't open /boot.\n");
    }
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
#ifdef SPRITED_LOCALDISK
    if (!fsDiskAttached) {
	ckfree((char *) defaultDiskPtr);
    }
#endif
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
     * This could be shared after forking to implement file descriptor 
     * sharing.  
     * 
     * If the state pointer is null, the file system hasn't been
     * initialized yet.  This should only happen early in the system
     * startup, for server threads that are needed to get the file system 
     * running (e.g., timer & network processing).
     */
    
    parFsPtr = parentProcPtr->fsPtr;
    if (parFsPtr == NULL) {
	if (!(newProcPtr->genFlags & PROC_KERNEL)) {
	    panic("Fs_InheritState: no FS state for user process.\n");
	}
	newProcPtr->fsPtr = NULL;
	return;
    }
    newProcPtr->fsPtr = newFsPtr = mnew(Fs_ProcessState);

    /*
     * Current working directory.
     */
    if (parFsPtr->cwdPtr != (Fs_Stream *)NIL) {
	Fsio_StreamCopy(parFsPtr->cwdPtr, &newFsPtr->cwdPtr);
    } else {
	newFsPtr->cwdPtr = (Fs_Stream *)NIL;
    }

    /*
     * Open stream list.
     */
    len = newFsPtr->numStreams = parFsPtr->numStreams;
    if (len > 0) {
	newFsPtr->streamList =
		(Fs_Stream **)ckalloc(len * sizeof(Fs_Stream *));
	newFsPtr->streamFlags = (char *)ckalloc(len * sizeof(char));
	for (i = 0, parStreamPtrPtr = parFsPtr->streamList,
		    newStreamPtrPtr = newFsPtr->streamList;
	     i < len;
	     i++, parStreamPtrPtr++, newStreamPtrPtr++) {
	    if (*parStreamPtrPtr != (Fs_Stream *) NIL) {
		Fsio_StreamCopy(*parStreamPtrPtr, newStreamPtrPtr);
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
	newFsPtr->groupIDs = (int *)ckalloc(len * sizeof(int));
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
 *	Note: to avoid problems when destroying a process, we have two
 *	phases of removing file system state.  Phase 0 will just close
 *	streams.  Phase 1 closes down everything.  (Phase 0 is optional.)
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
Fs_CloseState(procPtr, phase)
    Proc_LockedPCB *procPtr;		/* An exiting process to clean up */
    int phase;				/* 0 for start, 1 for total. */
{
    register int i;
    register Fs_ProcessState *fsPtr = procPtr->pcb.fsPtr;

    if (phase>0) {
	if (fsPtr->cwdPtr != (Fs_Stream *) NIL) {
	    (void)Fs_Close(fsPtr->cwdPtr);
	}
    }

    if (fsPtr->streamList != (Fs_Stream **)NIL) {
	for (i=0 ; i < fsPtr->numStreams ; i++) {
	    register Fs_Stream *streamPtr;

	    streamPtr = fsPtr->streamList[i];
	    if (streamPtr != (Fs_Stream *)NIL) {
		(void)Fs_Close(streamPtr);
	    }
	}
	ckfree((Address) fsPtr->streamList);
	ckfree((Address) fsPtr->streamFlags);
	fsPtr->streamList = (Fs_Stream **)NIL;
    }

    if (phase>0) {
	if (fsPtr->groupIDs != (int *) NIL) {
	    ckfree((Address) fsPtr->groupIDs);
	    fsPtr->groupIDs = (int *) NIL;
	    fsPtr->numGroupIDs = 0;
	}
	ckfree((Address)fsPtr);
	procPtr->pcb.fsPtr = (Fs_ProcessState *)NIL;
    }
}
