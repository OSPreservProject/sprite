/* 
 * fsTrace.c --
 *
 *	Subroutines for tracing file system events.
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
#endif not lint


#include <sprite.h>

#include <fs.h>
#include <vm.h>
#include <rpc.h>
#include <fsutil.h>
#include <fslcl.h>
#include <fsNameOps.h>
#include <fsprefix.h>
#include <fsutilTrace.h>
#include <fsStat.h>
#include <sync.h>
#include <timer.h>
#include <proc.h>
#include <trace.h>
#include <hash.h>
#include <fsrmt.h>

#include <stdio.h>

Trace_Header fsTraceHdr;
Trace_Header *fsutil_TraceHdrPtr = &fsTraceHdr;
int fsutil_TraceLength = 256;
Boolean fsutil_Tracing = FALSE;
Time fsTraceTime;		/* Cost of taking a trace record */
int fsutil_TracedFile = -1;		/* fileID.minor of traced file */

typedef struct FsTracePrintTable {
    Fsutil_TraceRecType	type;		/* This determines the format of the
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
    /* TRACE_GET_NEXT_FREE */	FST_HANDLE,	"get next free",
    /* TRACE_LRU_FREE */	FST_HANDLE,	"lru free",
    /* TRACE_LRU_DONE_FREE */	FST_HANDLE,	"lru done free",
};
static int numTraceTypes = sizeof(fsTracePrintTable)/sizeof(FsTracePrintTable);

/*
 *----------------------------------------------------------------------
 *
 * Fsutil_TraceInit --
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
Fsutil_TraceInit()
{
    Trace_Record *firstPtr, *lastPtr;

    Trace_Init(fsutil_TraceHdrPtr, fsutil_TraceLength, sizeof(Fsutil_TraceRecord), 0);
    /*
     * Take 10 trace records with a NIL data field.  This causes a
     * bzero inside Trace_Insert, which is sort of a base case for
     * taking a trace.  The time difference between the first and last
     * record is used to determine the cost of taking the trace records.
     */
    firstPtr = &fsutil_TraceHdrPtr->recordArray[fsutil_TraceHdrPtr->currentRecord];
    Trace_Insert(fsutil_TraceHdrPtr, 0, (ClientData)NIL);
    Trace_Insert(fsutil_TraceHdrPtr, 0, (ClientData)NIL);
    Trace_Insert(fsutil_TraceHdrPtr, 0, (ClientData)NIL);
    Trace_Insert(fsutil_TraceHdrPtr, 0, (ClientData)NIL);
    Trace_Insert(fsutil_TraceHdrPtr, 0, (ClientData)NIL);
    Trace_Insert(fsutil_TraceHdrPtr, 0, (ClientData)NIL);
    Trace_Insert(fsutil_TraceHdrPtr, 0, (ClientData)NIL);
    Trace_Insert(fsutil_TraceHdrPtr, 0, (ClientData)NIL);
    Trace_Insert(fsutil_TraceHdrPtr, 0, (ClientData)NIL);
    Trace_Insert(fsutil_TraceHdrPtr, 0, (ClientData)NIL);
    lastPtr = &fsutil_TraceHdrPtr->recordArray[fsutil_TraceHdrPtr->currentRecord-1];

    Time_Subtract(lastPtr->time, firstPtr->time, &fsTraceTime);
    Time_Divide(fsTraceTime, 9, &fsTraceTime);

    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Fsutil_PrintTraceRecord --
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
Fsutil_PrintTraceRecord(clientData, event, printHeaderFlag)
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
		    Fsutil_TraceIORec *ioRecPtr = (Fsutil_TraceIORec *)clientData;
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
		    Fsutil_TraceHdrRec *recPtr = (Fsutil_TraceHdrRec *)clientData;
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
		    Fsutil_TraceBlockRec *blockPtr = (Fsutil_TraceBlockRec *)clientData;
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
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Fsutil_PrintTrace --
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
Fsutil_PrintTrace(clientData)
    ClientData clientData;
{
    int numRecs = (int) clientData;

    if (numRecs < 0) {
	numRecs = fsutil_TraceLength;
    }
    printf("FS TRACE\n");
    (void)Trace_Print(fsutil_TraceHdrPtr, numRecs, Fsutil_PrintTraceRecord);
}
