/* 
 * fsSubr.c --
 *
 *	Miscellaneous routines.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"

#include "fs.h"
#include "vm.h"
#include "rpc.h"
#include "fsInt.h"
#include "fsDisk.h"
#include "fsLocalDomain.h"
#include "fsOpTable.h"
#include "fsPrefix.h"
#include "fsTrace.h"
#include "fsStat.h"
#include "devDiskLabel.h"
#include "dev.h"
#include "sync.h"
#include "timer.h"
#include "proc.h"
#include "trace.h"
#include "hash.h"

/*
 * TEMPORARY STUBS.
 */
Fs_RpcReply()
{
    panic( "Fs_RpcReply called");
}
Fs_RpcRequest()
{
    panic( "Fs_RpcRequest called");
}


#define	MAX_WAIT_INTERVALS	5

int	fsTimeInSeconds;
Boolean fsShouldSyncDisks;
Boolean	fsWriteThrough = FALSE;	
Boolean	fsWriteBackOnClose  = FALSE;
Boolean	fsDelayTmpFiles = FALSE;
int	fsTmpDirNum = -1;	
Boolean	fsWriteBackASAP = FALSE;
Boolean fsWBOnLastDirtyBlock = FALSE;

Trace_Header fsTraceHdr;
Trace_Header *fsTraceHdrPtr = &fsTraceHdr;
int fsTraceLength = 256;
Boolean fsTracing = TRUE;
Time fsTraceTime;		/* Cost of taking a trace record */
int fsTracedFile = -1;		/* fileID.minor of traced file */

typedef struct FsTracePrintTable {
    FsTraceRecType	type;		/* This determines the format of the
					 * trace record client data. */
    char		*string;	/* Human readable record type */
} FsTracePrintTable;

FsTracePrintTable fsTracePrintTable[] = {
    /* TRACE_0 */		FST_BLOCK, 	"delete block",
    /* TRACE_OPEN_START */	FST_NAME, 	"open start",
    /* TRACE_LOOKUP_START */	FST_NAME, 	"after prefix",
    /* TRACE_LOOKUP_DONE */	FST_NAME, 	"after lookup",
    /* TRACE_DEL_LAST_WR */	FST_HANDLE, 	"delete last writer",
    /* TRACE_OPEN_DONE */	FST_NAME, 	"open done",
    /* TRACE_BLOCK_WAIT */	FST_BLOCK,	"skip block",
    /* TRACE_BLOCK_HIT */	FST_BLOCK, 	"hit block",
    /* TRACE_DELETE */		FST_HANDLE, 	"delete",
    /* TRACE_NO_BLOCK */	FST_BLOCK, 	"new block",
    /* TRACE_OPEN_DONE_2 */	FST_NAME, 	"after Fs_Open",
    /* TRACE_OPEN_DONE_3 */	FST_NIL, 	"open complete",
    /* INSTALL_NEW */		FST_HANDLE,	"inst. new",
    /* INSTALL_HIT */		FST_HANDLE,	"inst. hit",
    /* RELEASE_FREE */		FST_HANDLE,	"rels. free",
    /* RELEASE_LEAVE */		FST_HANDLE,	"rels. leave",
    /* REMOVE_FREE */		FST_HANDLE,	"remv. free",
    /* REMOVE_LEAVE */		FST_HANDLE,	"remv. leave",
    /* SRV_WRITE_1 */		FST_IO,		"invalidate",
    /* SRV_WRITE_2 */		FST_IO,		"srv write",
    /* SRV_GET_ATTR_1 */	FST_NIL,	"srv get attr 1",
    /* SRV_GET_ATTR_2 */	FST_NIL,	"srv get attr 2",
    /* OPEN */			FST_NIL,	"open",
    /* READ */			FST_IO, 	"read",
    /* WRITE */			FST_IO, 	"write",
    /* CLOSE */			FST_HANDLE, 	"close",
    /* TRACE_RA_SCHED */	FST_RA, 	"Read ahead scheduled",
    /* TRACE_RA_BEGIN */	FST_RA,		"Read ahead started",
    /* TRACE_RA_END */		FST_RA,	 	"Read ahead completed",
    /* TRACE_DEL_BLOCK */	FST_BLOCK, 	"Delete block",
    /* TRACE_BLOCK_WRITE */	FST_BLOCK,	"Block write",
};
static int numTraceTypes = sizeof(fsTracePrintTable)/sizeof(FsTracePrintTable);

/*
 *----------------------------------------------------------------------
 *
 * FsTraceInit --
 *
 *	Initialize the filesystem trace record.  This also determines
 *	the cost of taking the trace records so this cost can be
 *	subtracted out when the records are displayed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Calls Trace_Init and Trace_Insert
 *
 *----------------------------------------------------------------------
 */
int
FsTraceInit()
{
    Trace_Record *firstPtr, *lastPtr;

    Trace_Init(fsTraceHdrPtr, fsTraceLength, sizeof(FsTraceRecord), 0);
    /*
     * Take 10 trace records with a NIL data field.  This causes a
     * bzero inside Trace_Insert, which is sort of a base case for
     * taking a trace.  The time difference between the first and last
     * record is used to determine the cost of taking the trace records.
     */
    firstPtr = &fsTraceHdrPtr->recordArray[fsTraceHdrPtr->currentRecord];
    Trace_Insert(fsTraceHdrPtr, 0, (ClientData)NIL);
    Trace_Insert(fsTraceHdrPtr, 0, (ClientData)NIL);
    Trace_Insert(fsTraceHdrPtr, 0, (ClientData)NIL);
    Trace_Insert(fsTraceHdrPtr, 0, (ClientData)NIL);
    Trace_Insert(fsTraceHdrPtr, 0, (ClientData)NIL);
    Trace_Insert(fsTraceHdrPtr, 0, (ClientData)NIL);
    Trace_Insert(fsTraceHdrPtr, 0, (ClientData)NIL);
    Trace_Insert(fsTraceHdrPtr, 0, (ClientData)NIL);
    Trace_Insert(fsTraceHdrPtr, 0, (ClientData)NIL);
    Trace_Insert(fsTraceHdrPtr, 0, (ClientData)NIL);
    lastPtr = &fsTraceHdrPtr->recordArray[fsTraceHdrPtr->currentRecord-1];

    Time_Subtract(lastPtr->time, firstPtr->time, &fsTraceTime);
    Time_Divide(fsTraceTime, 9, &fsTraceTime);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_PrintTraceRecord --
 *
 *	Format and print the client data part of a filesystem trace record.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	printfs to the display.
 *
 *----------------------------------------------------------------------
 */
int
Fs_PrintTraceRecord(clientData, event, printHeaderFlag)
    ClientData clientData;	/* Client data in the trace record */
    int event;			/* Type, or event, from the trace record */
    Boolean printHeaderFlag;	/* If TRUE, a header line is printed */
{
    if (printHeaderFlag) {
	/*
	 * Print column headers and a newline.
	 */
	printf("%10s %17s %8s\n", "Delta  ", "<File ID>  ", "Event ");
	printf("Cost of taking an fs trace record is %d.%06d\n",
		     fsTraceTime.seconds, fsTraceTime.microseconds);
    }
    if (clientData != (ClientData)NIL) {
	if (event >= 0 && event < numTraceTypes) {
	    printf("%20s ", fsTracePrintTable[event].string);
	    switch(fsTracePrintTable[event].type) {
		case FST_IO: {
		    FsTraceIORec *ioRecPtr = (FsTraceIORec *)clientData;
		    printf("<%2d, %2d, %1d, %4d> ",
			ioRecPtr->fileID.type, ioRecPtr->fileID.serverID,
			ioRecPtr->fileID.major, ioRecPtr->fileID.minor);
		    printf(" off %d len %d ", ioRecPtr->offset,
						  ioRecPtr->numBytes);
		    break;
		}
		case FST_NAME: {
		    char *name = (char *)clientData;
		    name[39] = '\0';
		    printf("\"%s\"", name);
		    break;
		}
		case FST_HANDLE: {
		    FsTraceHdrRec *recPtr = (FsTraceHdrRec *)clientData;
		    printf("<%2d, %2d, %1d, %4d> ref %d blocks %d ",
		      recPtr->fileID.type, 
		      recPtr->fileID.serverID,
		      recPtr->fileID.major, 
		      recPtr->fileID.minor,
		      recPtr->refCount,
		      recPtr->numBlocks);
		    break;
		}
		case FST_BLOCK: {
		    FsTraceBlockRec *blockPtr = (FsTraceBlockRec *)clientData;
		    printf("<%2d, %2d, %1d, %4d> block %d flags %x ",
		      blockPtr->fileID.type, 
		      blockPtr->fileID.serverID,
		      blockPtr->fileID.major, 
		      blockPtr->fileID.minor,
		      blockPtr->blockNum,
		      blockPtr->flags);
		      break;
		}
		case FST_RA: {
		    int	*blockNumPtr;

		    blockNumPtr = (int *) clientData;
		    printf("<Block=%d> ", *blockNumPtr);
		    break;
		}
		case FST_NIL:
		default:
		    break;
	    }
	} else {
	    printf("(%d)", event);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_PrintTrace --
 *
 *	Dump out the fs trace.
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
Fs_PrintTrace(numRecs)
    int numRecs;
{
    if (numRecs < 0) {
	numRecs = fsTraceLength;
    }
    printf("FS TRACE\n");
    (void)Trace_Print(fsTraceHdrPtr, numRecs, Fs_PrintTraceRecord);
}


int		fsWriteBackInterval = 30;	/* How long blocks have to be
						 * dirty before they are
						 * written back. */
int		fsWriteBackCheckInterval = 5;	/* How often to scan the
						 * cache for blocks to write
						 * back. */
Boolean		fsShouldSyncDisks = TRUE;	/* TRUE means that we should
						 * sync the disks when
						 * Fs_SyncProc is called. */
int		lastHandleWBTime = 0;		/* Last time that wrote back
						 * file handles. */
/*
 *----------------------------------------------------------------------
 *
 * Fs_SyncProc --
 *
 *	Process to loop and write back things every thiry seconds.
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
Fs_SyncProc(data, callInfoPtr)
    ClientData		data;		/* IGNORED */
    Proc_CallInfo	*callInfoPtr;
{
    int	blocksLeft;

    if (fsTimeInSeconds - lastHandleWBTime >= fsWriteBackInterval) {
	(void) FsHandleDescWriteBack(FALSE, -1);
	lastHandleWBTime = fsTimeInSeconds;
    }

    if (fsShouldSyncDisks && !fsWriteThrough && !fsWriteBackASAP) {
	Fs_CacheWriteBack((unsigned) (fsTimeInSeconds - fsWriteBackInterval), 
			  &blocksLeft, FALSE);
    }
    if (fsWriteBackCheckInterval < fsWriteBackInterval) {
	callInfoPtr->interval = fsWriteBackCheckInterval * timer_IntOneSecond;
    } else {
	callInfoPtr->interval = fsWriteBackInterval * timer_IntOneSecond;
    }

}


/*
 *----------------------------------------------------------------------
 *
 * Fs_Sync --
 *
 *	Write back bit maps, file descriptors, and all dirty cache buffers.
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
Fs_Sync(writeBackTime, shutdown)
    unsigned int writeBackTime;	/* Write back all blocks in the cache and file
			           descriptors that were dirtied before 
				   this time. */
    Boolean	shutdown;	/* TRUE if the kernel is being shutdown. */
{
    int		blocksLeft = 0;

    /*
     * Force all file descriptors into the cache.
     */
    (void) FsHandleDescWriteBack(shutdown, -1);
    /*
     * Write back the cache.
     */
    Fs_CacheWriteBack(writeBackTime, &blocksLeft, shutdown);
    if (shutdown) {
	if (blocksLeft) {
	    printf("Fs_Sync: %d blocks still locked\n", blocksLeft);
	}
	FsCleanBlocks((ClientData) FALSE, (Proc_CallInfo *) NIL);
    }
    /*
     * Finally write all domain information to disk.  This will mark each
     * domain to indicate that we went down gracefully and recovery is in
     * fact possible.
     */
    FsLocalDomainWriteBack(-1, shutdown, FALSE);
}

/*
 *----------------------------------------------------------------------
 *
 * FsUpdateTimeOfDay --
 *
 *	Update the time of day in seconds.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Global time of day variable updated.
 *
 *----------------------------------------------------------------------
 */

void
FsUpdateTimeOfDay()
{
    Time	time;

    Timer_GetTimeOfDay(&time, (int *) NIL, (Boolean *) NIL);
    fsTimeInSeconds = time.seconds;
    Timer_RescheduleRoutine(&fsTimeOfDayElement, TRUE);
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_CheckSetID --
 *
 *	Determine if the given stream has the set uid or set gid bits set.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	*uidPtr and *gidPtr set to -1 if the respective bit isn't set and set
 *	to the uid and/or gid of the file otherwise.
 *
 *----------------------------------------------------------------------
 */
void
Fs_CheckSetID(streamPtr, uidPtr, gidPtr)
    Fs_Stream	*streamPtr;
    int		*uidPtr;
    int		*gidPtr;
{
    register	FsCachedAttributes	*cachedAttrPtr;

    switch (streamPtr->ioHandlePtr->fileID.type) {
	case FS_LCL_FILE_STREAM:
	    cachedAttrPtr =
	       &((FsLocalFileIOHandle *)streamPtr->ioHandlePtr)->cacheInfo.attr;
	    break;
	case FS_RMT_FILE_STREAM:
	    cachedAttrPtr =
	       &((FsRmtFileIOHandle *)streamPtr->ioHandlePtr)->cacheInfo.attr;
	    break;
	default:
	    panic( "Fs_CheckSetID, wrong stream type\n",
		streamPtr->ioHandlePtr->fileID.type);
	    return;
    }
    if (cachedAttrPtr->permissions & FS_SET_UID) {
	*uidPtr = cachedAttrPtr->uid;
    } else {
	*uidPtr = -1;
    }
    if (cachedAttrPtr->permissions & FS_SET_GID) {
	*gidPtr = cachedAttrPtr->gid;
    } else {
	*gidPtr = -1;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FsDomainInfo --
 *
 *	Return info about the given domain.
 *
 * Results:
 *	A return status.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus 
FsDomainInfo(hdrPtr, domainInfoPtr)
    FsHandleHeader	*hdrPtr;	/* Handle from the prefix table */
    Fs_DomainInfo	*domainInfoPtr;
{
    ReturnStatus	status;

    switch (hdrPtr->fileID.type) {
	case FS_LCL_FILE_STREAM:
	    status = FsLocalDomainInfo(hdrPtr->fileID.major,
					domainInfoPtr);
	    break;
	case FS_RMT_FILE_STREAM:
	    status = FsRemoteDomainInfo(&hdrPtr->fileID, domainInfoPtr);
	    break;
	default:
	    status = FS_DOMAIN_UNAVAILABLE;
    }
    return(status);
}

/*
 *----------------------------------------------------------------------------
 *
 * Fs_GetFileHandle --
 *
 *	Return an opaque handle for a file, really a pointer to its I/O handle.
 *	This is used for a subsequent call to Fs_GetSegPtr.
 *
 * Results:
 *	A pointer to the I/O handle of the file.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 *
 */

ClientData
Fs_GetFileHandle(streamPtr)
    Fs_Stream *streamPtr;
{
    return((ClientData)streamPtr->ioHandlePtr);
}

/*
 *----------------------------------------------------------------------------
 *
 * Fs_GetSegPtr --
 *
 *	Return a pointer to a pointer to the segment associated with this
 *	file.
 *
 * Results:
 *	A pointer to the segment associated with this file.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 *
 */

Vm_Segment **
Fs_GetSegPtr(fileHandle)
    ClientData fileHandle;
{
    FsHandleHeader *hdrPtr = (FsHandleHeader *)fileHandle;
    Vm_Segment	**segPtrPtr;

    switch (hdrPtr->fileID.type) {
	case FS_LCL_FILE_STREAM:
	    segPtrPtr = &(((FsLocalFileIOHandle *)hdrPtr)->segPtr);
	    break;
	case FS_RMT_FILE_STREAM:
	    segPtrPtr = &(((FsRmtFileIOHandle *)hdrPtr)->segPtr);
	    break;
	default:
	    panic( "Fs_RetSegPtr, bad stream type %d\n",
		    hdrPtr->fileID.type);
    }
    fsStats.handle.segmentFetches++;
    if (*segPtrPtr != (Vm_Segment *) NIL) {
	fsStats.handle.segmentHits++;
    }
    return(segPtrPtr);
}

/*
 *----------------------------------------------------------------------------
 *
 * Fs_HandleScavengeStub --
 *
 *	This is a thin layer on top of Fs_HandleScavenge.  It is called
 *	when L1-x is pressed at the keyboard, and also from FsHandleInstall
 *	when a threashold number of handles have been created.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Invokes the handle scavenger.
 *
 *----------------------------------------------------------------------------
 *
 */
/*ARGSUSED*/
void Fs_HandleScavengeStub(data)
    ClientData	data;	/* IGNORED */
{
    /*
     * This is called when the L1-x keys are held down at the console.
     */
    Proc_CallFunc(Fs_HandleScavenge, (ClientData)FALSE, 0);
}

int		fsScavengeInterval = 2;			/* 2 Minutes */
int		fsLastScavengeTime = 0;
static	int	numScavengers = 0;
static	Boolean	scavengerStuck = FALSE;

/*
 * Set a threshold for when to warn about the scavenger getting stuck.
 * During high loads it can appear to get stuck for 0 or 1 seconds, or
 * more.  If the time is greater than SCAVENGE_WARNING_TIME, print
 * a warning.  SCAVENGE_WARNING_TIME is in seconds.
 */
#define SCAVENGE_WARNING_TIME 3


/*
 *----------------------------------------------------------------------------
 *
 * Fs_HandleScavenge --
 *
 *	Go through all of the handles looking for clients that have crashed
 *	and for handles that are no longer needed.  This expects to be
 *	called by a helper kernel processes at regular intervals defined
 *	by fsScavengeInterval.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The handle-specific routines may remove handles.
 *
 *----------------------------------------------------------------------------
 *
 */
/*ARGSUSED*/
void
Fs_HandleScavenge(data, callInfoPtr)
    ClientData		data;			/* Whether to reschedule again
						 */
    Proc_CallInfo	*callInfoPtr;		/* Specifies interval */
{
    Hash_Search				hashSearch;
    register	FsHandleHeader		*hdrPtr;

    if (numScavengers > 0) {
	if (!scavengerStuck &&
	    fsTimeInSeconds > fsLastScavengeTime + SCAVENGE_WARNING_TIME) {
	   printf( "Scavenger stuck for %d seconds\n",
	       fsTimeInSeconds - fsLastScavengeTime);
	   scavengerStuck = TRUE;
       }
       callInfoPtr->interval = 0;
       return;
    }
    numScavengers++;

    /*
     * Note that this is unsynchronized access to a global variable, which
     * works fine on a uniprocessor.  We don't want a monitor lock here
     * because we don't want a locked handle to hang up all Proc_ServerProcs.
     */
    fsLastScavengeTime = fsTimeInSeconds;

    Hash_StartSearch(&hashSearch);
    for (hdrPtr = FsGetNextHandle(&hashSearch);
	 hdrPtr != (FsHandleHeader *) NIL;
         hdrPtr = FsGetNextHandle(&hashSearch)) {
	 (*fsStreamOpTable[hdrPtr->fileID.type].scavenge)(hdrPtr);
    }
    /*
     * Set up our next call.
     */
    if ((Boolean)data && numScavengers == 1) {
	callInfoPtr->interval = fsScavengeInterval * timer_IntOneMinute;
    }
    if (scavengerStuck) {
	printf( "Scavenger unstuck after %d seconds\n",
	    fsTimeInSeconds - fsLastScavengeTime);
	scavengerStuck = FALSE;
    }
    numScavengers--;
}

/*
 *----------------------------------------------------------------------
 *
 * FsFileError --
 *
 *	Print an error message about a file.
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
FsFileError(hdrPtr, string, status)
    FsHandleHeader *hdrPtr;
    char *string;
{
    if (hdrPtr == (FsHandleHeader *)NIL) {
	printf("(NIL handle) %s: ", string);
    } else {
	Net_HostPrint(hdrPtr->fileID.serverID,
		      FsFileTypeToString(hdrPtr->fileID.type));
	printf(" \"%s\" <%d,%d> %s: ", FsHandleName(hdrPtr),
		hdrPtr->fileID.major, hdrPtr->fileID.minor, string);
    }
    switch (status) {
	case SUCCESS:
	    printf("\n");
	    break;
	case FS_DOMAIN_UNAVAILABLE:
	    printf("domain unavailable\n");
	    break;
	case FS_VERSION_MISMATCH:
	    printf("version mismatch\n");
	    break;
	case FAILURE:
	    printf("cacheable/busy conflict\n");
	    break;
	case RPC_TIMEOUT:
	    printf("rpc timeout\n");
	    break;
	case RPC_SERVICE_DISABLED:
	    printf("server rebooting\n");
	    break;
	case FS_STALE_HANDLE:
	    printf("stale handle\n");
	    break;
	case DEV_RETRY_ERROR:
	case DEV_HARD_ERROR:
	    printf("DISK ERROR\n");
	    break;
	case FS_NO_DISK_SPACE:
	    printf("out of disk space\n");
	default:
	    printf("<%x>\n", status);
	    break;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FsFileTypeToString --
 *
 *	Map a stream type to a string.  Used for error messages.
 *
 * Results:
 *	A string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
char *
FsFileTypeToString(type)
    int type;
{
    register char *fileType;

    switch (type) {
	case FS_STREAM:
	    fileType = "Stream";
	    break;
	case FS_LCL_FILE_STREAM:
	    fileType = "File";
	    break;
	case FS_RMT_FILE_STREAM:
	    fileType = "RmtFile";
	    break;
	case FS_LCL_DEVICE_STREAM:
	    fileType = "Device";
	    break;
	case FS_RMT_DEVICE_STREAM:
	    fileType = "RmtDevice";
	    break;
	case FS_LCL_PIPE_STREAM:
	    fileType = "Pipe";
	    break;
	case FS_RMT_PIPE_STREAM:
	    fileType = "RmtPipe";
	    break;
	case FS_LCL_NAMED_PIPE_STREAM:
	    fileType = "NamedPipe";
	    break;
	case FS_RMT_NAMED_PIPE_STREAM:
	    fileType = "RmtNamedPipe";
	    break;
	case FS_CONTROL_STREAM:
	    fileType = "PdevControlStream";
	    break;
	case FS_SERVER_STREAM:
	    fileType = "SrvStream";
	    break;
	case FS_LCL_PSEUDO_STREAM:
	    fileType = "LclPdev";
	    break;
	case FS_RMT_PSEUDO_STREAM:
	    fileType = "RmtPdev";
	    break;
	case FS_PFS_CONTROL_STREAM:
	    fileType = "PfsControlStream";
	    break;
	case FS_PFS_NAMING_STREAM:
	    fileType = "PfsNamingStream";
	    break;
	case FS_LCL_PFS_STREAM:
	    fileType = "LclPfs";
	    break;
	case FS_RMT_PFS_STREAM:
	    fileType = "RmtPfs";
	    break;

	case FS_RMT_UNIX_STREAM:
	    fileType = "UnixFile";
	    break;
	case FS_RMT_NFS_STREAM:
	    fileType = "NFSFile";
	    break;
	default:
	    fileType = "<unknown file type>";
	    break;
    }
    return(fileType);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_GetFileName --
 *
 *	Return a pointer to the file name for the given stream.
 *
 * Results:
 *	Pointer to file name from handle of given stream.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
char *
Fs_GetFileName(streamPtr)
    Fs_Stream	*streamPtr;
{
    if (streamPtr->hdr.name != (char *)NIL) {
	return(streamPtr->hdr.name);
    } else if (streamPtr->ioHandlePtr != (FsHandleHeader *)NIL) {
	return(streamPtr->ioHandlePtr->name);
    } else {
	return("(noname)");
    }
}
