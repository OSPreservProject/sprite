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
#include "mem.h"
#include "byte.h"
#include "timer.h"
#include "proc.h"
#include "trace.h"
#include "hash.h"

/*
 * TEMPORARY STUBS.
 */
Fs_RpcReply()
{
    Sys_Panic(SYS_FATAL, "Fs_RpcReply called");
}
Fs_RpcRequest()
{
    Sys_Panic(SYS_FATAL, "Fs_RpcRequest called");
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
int fsTraceLength = 50;
Boolean fsTracing = FALSE;
Time fsTraceTime;		/* Cost of taking a trace record */

typedef struct FsTracePrintTable {
    FsTraceRecType	type;		/* This determines the format of the
					 * trace record client data. */
    char		*string;	/* Human readable record type */
} FsTracePrintTable;

FsTracePrintTable fsTracePrintTable[] = {
    /* TRACE_0 */		FST_NIL, "zero",
    /* TRACE_1 */		FST_NAME, "after prefix",
    /* TRACE_2 */		FST_NAME, "after setup",
    /* TRACE_3 */		FST_NAME, "after scan",
    /* TRACE_4 */		FST_NAME, "after domain lookup",
    /* TRACE_5 */		FST_NAME, "after lookup",
    /* TRACE_6 */		FST_NIL, "six",
    /* TRACE_7 */		FST_NIL, "seven",
    /* TRACE_8 */		FST_NIL, "eight",
    /* TRACE_9 */		FST_NIL, "nine",
    /* TRACE_10 */		FST_NAME, "after Fs_Open",
    /* TRACE_11 */		FST_NIL, "open complete",
    /* SRV_OPEN_1 */		FST_NAME,	"srv open 1",
    /* SRV_OPEN_2 */		FST_HANDLE,	"srv open 2",
    /* SRV_CLOSE_1 */		FST_NIL,	"srv close 1",
    /* SRV_CLOSE_2 */		FST_HANDLE,	"srv close 2",
    /* SRV_READ_1 */		FST_IO,		"srv read 1",
    /* SRV_READ_2 */		FST_HANDLE,	"srv read 2",
    /* SRV_WRITE_1 */		FST_IO,		"srv write 1",
    /* SRV_WRITE_2 */		FST_HANDLE,	"srv write 2",
    /* SRV_GET_ATTR_1 */	FST_NIL, "srv get attr 1",
    /* SRV_GET_ATTR_2 */	FST_NIL, "srv get attr 2",
    /* OPEN */			FST_NIL, "open",
    /* READ */			FST_NIL, "read",
    /* WRITE */			FST_NIL, "write",
    /* CLOSE */			FST_NIL, "close",
    /* TRACE_RA_SCHED */	FST_RA, "Read ahead scheduled",
    /* TRACE_RA_BEGIN */	FST_RA, "Read ahead started",
    /* TRACE_RA_END */		FST_RA, "Read ahead completed",
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
     * Byte_Zero inside Trace_Insert, which is sort of a base case for
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
 *	Sys_Printf to the display.
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
	Sys_Printf("%10s %17s %8s\n", "Delta  ", "<File ID>  ", "Event ");
	Sys_Printf("Cost of taking an fs trace record is %d.%06d\n",
		     fsTraceTime.seconds, fsTraceTime.microseconds);
    }
    if (clientData != (ClientData)NIL) {
	if (event >= 0 && event < numTraceTypes) {
	    Sys_Printf("%20s", fsTracePrintTable[event].string);
	    switch(fsTracePrintTable[event].type) {
		case FST_IO: {
		    FsTraceIORec *ioRecPtr = (FsTraceIORec *)clientData;
		    Sys_Printf("<%2d, %2d, %1d, %4d> ",
			ioRecPtr->fileID.type, ioRecPtr->fileID.serverID,
			ioRecPtr->fileID.major, ioRecPtr->fileID.minor);
		    Sys_Printf(" off %d len %d ", ioRecPtr->offset,
						  ioRecPtr->numBytes);
		    break;
		}
		case FST_NAME: {
		    char *name = (char *)clientData;
		    name[39] = '\0';
		    Sys_Printf("\"%s\"", name);
		    break;
		}
		case FST_HANDLE: {
		    FsHandleHeader *hdrPtr = (FsHandleHeader *)clientData;
		    Sys_Printf("<%2d, %2d, %1d, %4d> ",
		      hdrPtr->fileID.type, 
		      hdrPtr->fileID.serverID,
		      hdrPtr->fileID.major, 
		      hdrPtr->fileID.minor);
		    break;
		}
		case FST_RA: {
		    int	*blockNumPtr;

		    blockNumPtr = (int *) clientData;
		    Sys_Printf("<Block=%d> ", *blockNumPtr);
		    break;
		}
		case FST_NIL:
		default:
		    break;
	    }
	} else {
	    Sys_Printf("(%d)", event);
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
    Sys_Printf("FS TRACE\n");
    Trace_Print(fsTraceHdrPtr, numRecs, Fs_PrintTraceRecord);
}


Timer_Ticks 	writeBackSleep;
int		fsWriteBackInterval = 30;
Boolean		fsShouldSyncDisks = TRUE;

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
    if (fsShouldSyncDisks) {
	Fs_Sync((unsigned int) (fsTimeInSeconds - fsWriteBackInterval), 
		FALSE);
    }
    callInfoPtr->interval = fsWriteBackInterval * timer_IntOneSecond;
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
    int		blocksLeft;

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
	    Sys_Printf("Fs_Sync: %d blocks still locked\n", blocksLeft);
	}
	FsCleanBlocks((ClientData) FALSE, (Proc_CallInfo *) NIL);
    }
    /*
     * Finally write all domain information to disk.  If we are shutting down
     * the system this will mark each domain to indicate that we went down
     * gracefully and recovery is in fact possible.
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
	    Sys_Panic(SYS_FATAL, "Fs_CheckSetID, wrong stream type\n",
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
	    status = FsSpriteDomainInfo(&hdrPtr->fileID, domainInfoPtr);
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
	    Sys_Panic(SYS_FATAL, "Fs_RetSegPtr, bad stream type %d\n",
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
 * Fs_HandleScavenge --
 *
 *	Go through all of the handles looking for clients that have crashed
 *	and for handles that are no longer needed.  This expects to be
 *	called by a helper kernel processes at regular intervals defined
 *	by fsScavengeInterval.
 *
 *	Note: Fs_HandleScavengeStub is called via L1-x from the keyboard
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 *
 */
int		fsScavengeInterval = 2;			/* 2 Minutes */
static int	lastScavengeTime = 0;

/*ARGSUSED*/
void Fs_HandleScavengeStub(data)
    ClientData	data;	/* IGNORED */
{
    /*
     * This is called when the L1-x keys are held down at the console.
     */
    Proc_CallFunc(Fs_HandleScavenge, (ClientData)FALSE, 0);
}
/*ARGSUSED*/
void
Fs_HandleScavenge(data, callInfoPtr)
    ClientData		data;			/* Whether to reschedule again
						 */
    Proc_CallInfo	*callInfoPtr;		/* Specifies interval */
{
    Hash_Search				hashSearch;
    register	FsHandleHeader		*hdrPtr;

    lastScavengeTime = fsTimeInSeconds;

    Hash_StartSearch(&hashSearch);
    for (hdrPtr = FsGetNextHandle(&hashSearch);
	 hdrPtr != (FsHandleHeader *) NIL;
         hdrPtr = FsGetNextHandle(&hashSearch)) {
	 (*fsStreamOpTable[hdrPtr->fileID.type].scavenge)(hdrPtr);
    }
    /*
     * Set up our next call.
     */
    if ((Boolean)data) {
	callInfoPtr->interval = fsScavengeInterval * timer_IntOneMinute;
    }
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
    switch (hdrPtr->fileID.type) {
	case FS_STREAM:
	    Sys_Printf("Stream ");
	    break;
	case FS_LCL_FILE_STREAM:
	    Sys_Printf("File ");
	    break;
	case FS_RMT_FILE_STREAM:
	    Sys_Printf("RmtFile ");
	    break;
	case FS_LCL_DEVICE_STREAM:
	    Sys_Printf("Device ");
	    break;
	case FS_RMT_DEVICE_STREAM:
	    Sys_Printf("RmtDevice ");
	    break;
	case FS_LCL_PIPE_STREAM:
	    Sys_Printf("Pipe ");
	    break;
	case FS_RMT_PIPE_STREAM:
	    Sys_Printf("RmtPipe ");
	    break;
	case FS_LCL_NAMED_PIPE_STREAM:
	    Sys_Printf("NamedPipe ");
	    break;
	case FS_RMT_NAMED_PIPE_STREAM:
	    Sys_Printf("RmtNamedPipe ");
	    break;
	case FS_CONTROL_STREAM:
	    Sys_Printf("ControlStream ");
	    break;
	case FS_SERVER_STREAM:
	    Sys_Printf("SrvStream ");
	    break;
	case FS_LCL_PSEUDO_STREAM:
	    Sys_Printf("LclPdev ");
	    break;
	case FS_RMT_PSEUDO_STREAM:
	    Sys_Printf("RmtPdev ");
	    break;
	case FS_RMT_UNIX_STREAM:
	    Sys_Printf("UnixFile ");
	    break;
	case FS_RMT_NFS_STREAM:
	    Sys_Printf("NFSFile ");
	    break;
    }
    Sys_Printf("<%d,%d> server %d: %s", hdrPtr->fileID.major,
	    hdrPtr->fileID.minor, hdrPtr->fileID.serverID, string);
    switch (status) {
	case FS_DOMAIN_UNAVAILABLE:
	    Sys_Printf("domain unavailable\n");
	    break;
	case FS_VERSION_MISMATCH:
	    Sys_Printf("version mismatch\n");
	    break;
	case FAILURE:
	    Sys_Printf("cacheable/busy conflict\n");
	    break;
	case RPC_TIMEOUT:
	    Sys_Printf("rpc timeout\n");
	    break;
	case RPC_SERVICE_DISABLED:
	    Sys_Printf("server rebooting\n");
	    break;
	case FS_STALE_HANDLE:
	    Sys_Printf("stale handle\n");
	    break;
	case DEV_RETRY_ERROR:
	case DEV_HARD_ERROR:
	    Sys_Printf("DISK ERROR\n");
	    break;
	case FS_NO_DISK_SPACE:
	    Sys_Printf("out of disk space\n");
	default:
	    Sys_Printf("<%x>\n", status);
	    break;
    }

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
    return(streamPtr->nameInfoPtr->name);
}
