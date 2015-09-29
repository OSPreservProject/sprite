/* 
 * fsattach.c --
 *
 *	Boot-time program to attach all disks to a server.  This
 *	uses a mount file that indicates what prefixes/disks to
 *	attach.  This also siphons off some availability/reliability
 *	information that is put in the summary information sector
 *	on each disk.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/admin/fsattach/RCS/fsattach.c,v 1.10 91/01/12 16:48:11 jhh Exp $ SPRITE (Berkeley)";
#endif not lint

#include "fsattach.h"

char *mountFile = "mount";
char *devDir = "/dev/";
char *fscheck = "fscheck";
int verboseLevel = -1;
Boolean nomount = FALSE;
Boolean sequential = FALSE;
Boolean printOnly = FALSE;
Boolean writeDisk = TRUE;
Boolean fastboot = FALSE;
Boolean debug = FALSE;
int	spriteID = 0;
int	maxChildren = -1;
Boolean condCheck = FALSE;

Option optionArray[] = {
    {OPT_STRING,   "d", (char *) &devDir,
	"Device directory."},
    {OPT_STRING,   "fscheck", (char *) &fscheck,
	"fscheck program."},
    {OPT_TRUE,   "f", (char *) &fastboot,
	"Don't check disks."},
    {OPT_INT,   "i", (char *) &spriteID,
	"Preload prefix table with ourself as server of prefixes we export"},
    {OPT_INT,   "j", (char *) &maxChildren,
	"Maximum number of fscheck jobs to run at a time"},
    {OPT_TRUE,   "k", (char *) &debug,
	"Print debugging output."},
    {OPT_STRING,   "m", (char *) &mountFile,
	"File containing disk<=>prefix information."},
    {OPT_TRUE,   "n", (char *) &nomount,
	"No mount."},
    {OPT_TRUE,   "p", (char *) &printOnly,
	"Don't do anything. Just print out actions."},
    {OPT_TRUE,   "s", (char *) &sequential,
	"Ignore group information and run fscheck sequentially."},
    {OPT_TRUE,   "v", (char *) &verbose,
	"Verbose output from fsattach and fscheck."},
    {OPT_FALSE,   "W", (char *) &writeDisk,
	"Don't let fscheck write to the disks."},
    {OPT_TRUE, "c", (char *) &condCheck,
	"Conditionally check the disks (don't re-check)."},

};
int numOptions = Opt_Number(optionArray);

char		*progName;
int		returnCode;

int 		mountTableSize = 30;
int		mountTableSizeIncrement = 5;
MountInfo	*mountTable;
int		groupInfoSize = 10;
int		groupInfoSizeIncrement = 2;
GroupInfo	*groupInfo;
int		numGroups;
char		*tempOutputFile = ".fscheck.out";
int		tempOutputFileSize = 8192;
char		*heapLimitString = "1000000";
Boolean		verbose = FALSE;
Boolean 	reboot = FALSE;




/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Main program for "fsattach":  attach disks at boottime.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

main(argc, argv)
    int argc;
    char *argv[];
{
    int 		mountCount = 0;
    int 		i;
    int 		j;
    ReturnStatus	status;
    int			numChecks;

    argc = Opt_Parse(argc, argv, optionArray, numOptions, 0);

    returnCode = OK;
    progName = argv[0];
    Alloc(mountTable, MountInfo, mountTableSize, "mountTable");
    assert(mountTable != NULL);
    for (i = 0; i < mountTableSize; i++) {
	mountTable[i].status = CHILD_OK;
    }
    Alloc(groupInfo, GroupInfo, groupInfoSize, "groupInfo");
    assert(groupInfo != NULL);
    bzero(groupInfo, sizeof(GroupInfo) * groupInfoSize);
    strcpy(groupInfo[0].name, "root");
    numGroups = 1;
    status = ParseMount(mountFile, &mountCount);
    if (status != SUCCESS) {
	exit(HARDERROR);
    }
    if (spriteID != 0) {
	PreloadPrefixTable(spriteID, mountCount);
    }
    /*
     * We don't want to check the same partition twice. If two entries in
     * the parse table have the same source then set doCheck on one to
     * false. Also count how many partitions have to be done in each pass.
     */
    numChecks = 0;
    for (i = 0; i < mountCount; i++) {
	for (j = i+1; j < mountCount; j++) {
	    if (!strcmp(mountTable[i].source, mountTable[j].source)) {
		mountTable[j].doCheck = FALSE;
	    }
	}
	if (mountTable[i].doCheck == TRUE) {
	    numChecks++;
	}
    }
    if (!fastboot) {
	CacheWriteBack(FALSE);
	CheckDisks(mountCount, numChecks);
	if (!reboot) {
	    CacheWriteBack(TRUE);
	}
    }
    if (reboot) {
	exit(REBOOT);
    }
    if (!nomount) {
	Prefix(mountCount);
    }
    MoveOutput(mountCount);

    if (debug) {
	printf("(1) Exiting with %d.\n", returnCode);
    }
    (void) exit(returnCode);
}

/*
 *----------------------------------------------------------------------
 *
 * CheckDisks --
 *
 *	Check the disks. For each pass fork of an fscheck process for
 *	each partition to be checked.
 *
 * Results:
 *	FAILURE if an error occurred, SUCCESS otherwise.
 *
 * Side effects:
 *	MountTable entries are modified.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
CheckDisks(mountCount, numChecks)
    int			mountCount;	/* # entries in mount table */
    int			numChecks;	/* # of partitions to check */
{
    int			i;
    ChildInfo		*childInfo;
    int			pass;
    int			doneChildren;
    int			activeChildren;
    union wait 		waitStatus;
    ReturnStatus	status;
    int			childPid;
    MountInfo		*mountPtr;

    Alloc(childInfo, ChildInfo, numChecks, "childInfo");
    assert(childInfo != NULL);
    for (i = 0; i < numChecks; i++) {
	childInfo[i].pid = -1;
    }
    activeChildren = 0;
    doneChildren = 0;
    if (maxChildren < 0) {
	maxChildren = numGroups;
    }
    while (doneChildren < numChecks) {
	while (activeChildren < maxChildren) {
	    status = RunChild(mountCount, numChecks, childInfo, &i);
	    if (status == FAILURE) {
		if (i < 0) {
		    /*
		     * There are no available jobs that can be run.
		     * Wait for some to finish.
		     */
		    assert(activeChildren > 0);
		    break;
		}
		/*
		 * Some sort of error occurred.
		 */
		mountTable[i].doCheck = FALSE;
		returnCode = HARDERROR;
		doneChildren++;
	    } else {
		mountTable[i].status = CHILD_RUNNING;
		groupInfo[mountTable[i].group].running = TRUE;
		activeChildren++;
		if (sequential) {
		    break;
		}
	    }
	}
	if (activeChildren > 0) {
	    if (printOnly) {
		for (i = 0; i < mountCount; i++) {
		    if (mountTable[i].status == CHILD_RUNNING) {
			break;
		    }
		}
		assert(i != numChecks);
		childPid = i;
	    } else {
		childPid = wait(&waitStatus);
	    }
	    activeChildren--;
	    for (i = 0; i < numChecks; i++) {
		if (childInfo[i].pid == childPid) {
		    mountPtr = &(mountTable[childInfo[i].mountIndex]);
		    childInfo[i].pid = -1;
		    break;
		}
	    }
	    if (verbose) {
		printf("Fscheck of %s finished.\n", mountPtr->source);
	    }
	    if (printOnly) {
		doneChildren++;
		mountPtr->status = CHILD_OK;
		mountPtr->doCheck = FALSE;
		groupInfo[mountPtr->group].running = FALSE;
		continue;
	    }
	    if (WIFEXITED(waitStatus) && 
		waitStatus.w_retcode == FSCHECK_OUT_OF_MEMORY) {
		continue;
	    }
	    doneChildren++;
	    mountPtr->doCheck = FALSE;
	    mountPtr->checked = TRUE;
	    groupInfo[mountPtr->group].running = FALSE;
	    if (WIFSIGNALED(waitStatus) || WIFSTOPPED(waitStatus)) {
		(void) fprintf(stderr,"%s did not finish.\n", fscheck);
		returnCode = HARDERROR;
		mountPtr->status = CHILD_FAILURE;
	    } else if (waitStatus.w_retcode == EXEC_FAILED) {
		returnCode = HARDERROR;
		mountPtr->status = CHILD_FAILURE;
	    } else {
		if (debug) {
		    printf("%s returned 0x%x.\n", fscheck,
			(unsigned int) waitStatus.w_retcode);
		    printf("returnCode is %d.\n", returnCode);
		}
		mountPtr->status = CHILD_OK;
		if ((char) waitStatus.w_retcode < 0 ) {
		    PrintFscheckError((char)waitStatus.w_retcode, mountPtr);
		    mountPtr->status = CHILD_FAILURE;
		    returnCode = HARDERROR;
		} else if ((char) waitStatus.w_retcode > 0) {
		    PrintFscheckError((char)waitStatus.w_retcode, mountPtr);
		    if ((char) waitStatus.w_retcode == FSCHECK_REBOOT) {
			reboot = TRUE;
		    }
		    if (returnCode == OK) {
			returnCode = SOFTERROR;
		    }
		} else if (verbose) {
		    PrintFscheckError((char)waitStatus.w_retcode, mountPtr);
		}
	    }
	}
    }
    assert(doneChildren == numChecks);
    assert(activeChildren == 0);
}

/*
 *----------------------------------------------------------------------
 *
 * CacheWriteBack --
 *
 *	Turns cache write-back on and off
 * Results:
 *	None.
 *
 * Side effects:
 *	The cache write-back status is changed.
 *
 *----------------------------------------------------------------------
 */

void
CacheWriteBack(value)
    int		value;
{

    int			newValue;
    ReturnStatus	status;
    int			lockedBlocks;

    if (printOnly && verbose) {
	printf("Setting cache write-back to %d.\n", value);
	return;
    }
    if (!writeDisk) {
	if (verbose) {
	    fprintf(stderr, 
		    "Fscheck not writing disks -- not changing  write-back.\n");
	}
	return;
    }
    newValue = value;
    status = Fs_Command(FS_DISABLE_FLUSH, sizeof(int), (Address) &value);
    if (status != SUCCESS) {
	(void) fprintf(stderr, "Fs_Command (1)  returned %d.\n", status);
	(void) exit(HARDERROR);
    }
    if (verbose) {
	(void) fprintf(stderr, "Cache write-back %s, was %s.\n", 
		(newValue) ? "on" : "off",
	        (value) ? "on" : "off");
    }
    /*
     * If we're turning the write-back off flush what's in the cache already.
     */
    if (newValue == 0) {
	status = Fs_Command(FS_EMPTY_CACHE, sizeof(int), 
		    (Address) &lockedBlocks);
	if (status != SUCCESS) {
	    (void) fprintf(stderr, "Fs_Command (2)  returned %d.\n", status);
	    (void) exit(HARDERROR);
	}
	if (lockedBlocks > 0) {
	    fprintf(stderr, "There are %d locked blocks in the cache ?!\n", 
		lockedBlocks);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * RunChild --
 *
 *	Forks a process to run fscheck.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A process is forked.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
RunChild(mountCount, numChecks, childInfo, mountIndexPtr)
    int		mountCount;		/* # entries in mount table */
    int		numChecks;		/* # of jobs to run */
    ChildInfo	*childInfo;		/* info on running children */
    int		*mountIndexPtr;		/* ptr to index into mount table */
{
    int		mountIndex;
    int		i;
    MountInfo	*mountPtr = NULL;
    static char	deviceName[MAX_FIELD_LENGTH];
    static char	partition[MAX_FIELD_LENGTH];
    static char	outputFile[MAX_FIELD_LENGTH];
    int		pid;
    ArgHeader	*argPtr;
    Boolean	defaultOutputFile = TRUE;
    Boolean	defaultRootPart = TRUE;
    Boolean	defaultDir = TRUE;

    /*
     * Find a child to run.
     */
    *mountIndexPtr = -1;
    for (mountIndex = 0; mountIndex < mountCount; mountIndex++) {
	if (mountTable[mountIndex].group == 0 &&
	    mountTable[mountIndex].status == CHILD_RUNNING) {
	    break;
	}
	if ((mountTable[mountIndex].doCheck) &&
	    (groupInfo[mountTable[mountIndex].group].running == FALSE) &&
	    (mountTable[mountIndex].status != CHILD_RUNNING)) {
	    mountPtr = &(mountTable[mountIndex]);
	    break;
	}
    }
    if (mountPtr == NULL) {
	return FAILURE;
    }
    *mountIndexPtr = mountIndex;
    /*
     * Put together the arguments to fscheck.
     */
    (void) strcpy(partition, &(mountPtr->source[strlen(mountPtr->source) -1 ]));
    (void) strcpy(deviceName, mountPtr->source);
    deviceName[strlen(mountPtr->source) - 1] = '\0';
    StartExec(fscheck, fscheck);
    AddExecArgs("-dev", deviceName, NULL);
    AddExecArgs("-part", partition, NULL);
    LIST_FORALL((List_Links *) &mountPtr->argInfo.argList, 
	(List_Links *) argPtr) {
	if (!strcmp("-verbose", argPtr->arg) && verbose) {
	    continue;
	} else if (!strcmp(argPtr->arg, "-write") && !writeDisk) {
	    continue;
	} else if (!strcmp("-dir", argPtr->arg)) {
	    defaultDir = FALSE;
	}
	AddExecArgs(argPtr->arg, NULL);
    }
    DeleteList(&mountPtr->argInfo.argList);
    if (verbose) {
	AddExecArgs("-verbose", NULL);
    }
    if (writeDisk) {
	AddExecArgs("-write", NULL);
    }
    AddExecArgs("-outputFile", tempOutputFile, NULL);
    AddExecArgs("-rawOutput", NULL);
    if (condCheck) {
	AddExecArgs("-cond", "-setCheck", NULL);
    }
    if (defaultDir) {
	AddExecArgs("-dir", devDir, NULL);
    }
    pid = DoExec();
    if (pid < 0) {
	if (verbose) {
	    fprintf(stderr, "Fork of child failed.\n");
	    perror("");
	}
	return FAILURE;
    }
    /*
     * Store info about the running child.
     */
    for (i = 0; i < numChecks; i++) {
	if (childInfo[i].pid == -1) {
	    if (printOnly) {
		childInfo[i].pid = mountIndex;
	    } else {
		childInfo[i].pid = pid;
	    }
	    childInfo[i].mountIndex = mountIndex;
	    break;
	}
    }
    assert(i != numChecks);
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * Prefix --
 *
 *	Adds all devices that were checked correctly to the prefix table.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Entries are added to the system prefix table.
 *
 *----------------------------------------------------------------------
 */

void
Prefix(count)
    int		count;		/* # entries in mount table */
{
    int			i;
    ReturnStatus	status;
    int			flags;
    Fs_TwoPaths		paths;
    char		buffer[128];
    char		*source;
    char		*dest;
    Boolean		device;
    char		*prefix;

    for (i = 0; i < count; i ++) {
	if (mountTable[i].status == CHILD_OK) {
	    flags = 0;
	    if (mountTable[i].readonly) {
		flags |= FS_ATTACH_READ_ONLY;
	    }
	    if (mountTable[i].export == FALSE) {
		flags |= FS_ATTACH_LOCAL;
	    }
	    source = mountTable[i].source;
	    dest = mountTable[i].dest;
	    device = mountTable[i].device;
	    /*
	     * Use Fs_AttachDisk to attach a disk.
	     */
	    if (device == TRUE) {
		printf("Attaching %s%s as %s.\n", devDir, source, dest);
		printf("%s %s.\n", mountTable[i].readonly ? "R" : "RW",
		       mountTable[i].export ? "Export" : "Local");
		if (printOnly) {
		    continue;
		}
		sprintf(buffer, "%s%s", devDir, source);

		status = Fs_AttachDisk(buffer, dest, flags);
		if (status == FS_DOMAIN_UNAVAILABLE) {
		    printf("%s is already attached.\n", buffer);
		    prefix = GetAttachName(buffer);
		    if (prefix != NULL) {
			source = prefix;
			device = FALSE;
		    }
		} else if (status != SUCCESS) {
		    (void) fprintf(stderr, "Attach \"%s\" on \"%s\": %s\n", 
			    buffer, dest, Stat_GetMsg(status));
		}
	    }
	    if (device == FALSE) {
		printf("Exporting %s as %s.\n", source, dest);
		if (printOnly) {
		    continue;
		}
		paths.pathLen1 = strlen(source) +1;
		paths.path1 = source;
		paths.pathLen2 = strlen(dest) +1;
		paths.path2 = dest;
		status = Fs_Command(FS_PREFIX_EXPORT, sizeof(paths), 
			            (Address) &paths);
		if (status != SUCCESS) {
		    fprintf(stderr, 
			    "Couldn't export  \"%s\" as \"%s\": %s\n",
			    source, dest, Stat_GetMsg(status));
		}
	    }
	}
    }
}
