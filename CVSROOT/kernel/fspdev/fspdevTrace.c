/* 
 * fsPdevTrace.c --
 *
 *	Routines for tracing the pseudo-device request-response protocol.
 *
 * Copyright 1987, 1988 Regents of the University of California
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
/*
 * Prevent tracing by defining CLEAN here before this next include
 */
#undef CLEAN
#include "fsPdev.h"
Boolean fsPdevDebug = FALSE;		/* Turns on print statements */
Trace_Header pdevTraceHdr;
Trace_Header *pdevTraceHdrPtr = &pdevTraceHdr;
int pdevTraceLength = 200;
Boolean pdevTracing = TRUE;		/* Turns on circular trace */
int pdevMaxTraceDataSize;
int pdevTraceIndex = 0;


/*
 *----------------------------------------------------------------------------
 *
 * FsPdevTraceInit --
 *
 *	Initialize the pseudo-device trace buffer.  Used for debugging
 *	and profiling.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Calls to the Trace module to allocate the trace buffer, etc.
 *
 *----------------------------------------------------------------------------
 *
 */
ReturnStatus
FsPdevTraceInit()
{
    Trace_Init(pdevTraceHdrPtr, pdevTraceLength, sizeof(PdevTraceRecord),
		TRACE_NO_TIMES);
}

/*
 *----------------------------------------------------------------------------
 *
 * Fs_PdevPrintRec --
 *
 *	Print a record of the pseudo-device trace buffer.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	printf's
 *
 *----------------------------------------------------------------------------
 *
 */
ReturnStatus
Fs_PdevPrintRec(clientData, event, printHeaderFlag)
    ClientData clientData;	/* Client data in the trace record */
    int event;			/* Type, or event, from the trace record */
    Boolean printHeaderFlag;	/* If TRUE, a header line is printed */
{
    PdevTraceRecord *recPtr = (PdevTraceRecord *)clientData;
    PdevTraceRecType pdevEvent = (PdevTraceRecType)event;
    if (printHeaderFlag) {
	/*
	 * Print column headers and a newline.
	 */
	printf("%6s %17s %8s\n", "REC", "  <File ID>  ", " Event ");
    }
    if (recPtr != (PdevTraceRecord *)NIL) {
	/*
	 * Print out the fileID that's part of each record.
	 */
	printf("%5d| ", recPtr->index);
	printf("<%8x,%8x> ",
	  recPtr->fileID.major, recPtr->fileID.minor);

	switch(pdevEvent) {
	    case PDEVT_SRV_OPEN:
		printf("Srv Open");
		printf(" refs %d writes %d",
			    recPtr->un.use.ref,
			    recPtr->un.use.write);
		break;
	    case PDEVT_CLT_OPEN:
		printf("Clt Open");
		printf(" refs %d writes %d",
			    recPtr->un.use.ref,
			    recPtr->un.use.write);
		 break;
	    case PDEVT_SRV_CLOSE:
		printf("Srv Close");
		printf(" refs %d writes %d",
			    recPtr->un.use.ref,
			    recPtr->un.use.write);
		 break;
	    case PDEVT_CLT_CLOSE:
		printf("Clt Close");
		printf(" refs %d writes %d",
			    recPtr->un.use.ref,
			    recPtr->un.use.write);
		 break;
	    case PDEVT_SRV_READ:
		printf("Srv Read"); break;
	    case PDEVT_SRV_READ_WAIT:
		printf("Srv Read Blocked"); break;
	    case PDEVT_SRV_SELECT:
		printf("Srv Select Wait"); break;
	    case PDEVT_SRV_WRITE:
		printf("Srv Write"); break;
	    case PDEVT_CNTL_READ:
		printf("Control Read"); break;
	    case PDEVT_READ_WAIT:
		printf("Wait for Read"); break;
	    case PDEVT_WAIT_LIST:
		printf("Wait List Notify"); break;
	    case PDEVT_SELECT: {
		printf("Select "); 
		if (recPtr != (PdevTraceRecord *)NIL ) {
		    if (recPtr->un.selectBits & FS_READABLE) {
			printf("R");
		    }
		    if (recPtr->un.selectBits & FS_WRITABLE) {
			printf("W");
		    }
		    if (recPtr->un.selectBits & FS_EXCEPTION) {
			printf("E");
		    }
		}
		break;
	    }
	    case PDEVT_WAKEUP: {
		/*
		 * Print the process ID from the wait info,
		 * and the select bits stashed in the wait info token.
		 */
		printf("Wakeup");
		if (recPtr != (PdevTraceRecord *)NIL ) {
		    printf(" %x ", recPtr->un.wait.procID);
		    if (recPtr->un.wait.selectBits & FS_READABLE) {
			printf("R");
		    }
		    if (recPtr->un.wait.selectBits & FS_WRITABLE) {
			printf("W");
		    }
		    if (recPtr->un.wait.selectBits & FS_EXCEPTION) {
			printf("E");
		    }
		}
		break;
	    }
	    case PDEVT_REQUEST: {
		printf("Request");
		if (recPtr != (PdevTraceRecord *)NIL) {
		    switch(recPtr->un.requestHdr.operation) {
			case PDEV_OPEN:
			    printf(" OPEN"); break;
#ifdef notdef
			case PDEV_DUP:
			    printf(" DUP"); break;
#endif notdef
			case PDEV_CLOSE:
			    printf(" CLOSE"); break;
			case PDEV_READ:
			    printf(" READ"); break;
			case PDEV_WRITE:
			    printf(" WRITE"); break;
			case PDEV_WRITE_ASYNC:
			    printf(" WRITE_ASYNC"); break;
			case PDEV_IOCTL:
			    printf(" IOCTL"); break;
			default:
			    printf(" ??"); break;
		    }
		}
		break;
	    }
	    case PDEVT_REPLY: {
		printf("Reply");
		if (recPtr != (PdevTraceRecord *)NIL) {
		    printf(" <%x> ", recPtr->un.reply.status);
		    if (recPtr->un.reply.selectBits & FS_READABLE) {
			printf("R");
		    }
		    if (recPtr->un.reply.selectBits & FS_WRITABLE) {
			printf("W");
		    }
		}
		break;
	    }
	    default:
		printf("<%d>", event); break;

	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_PdevPrintTrace --
 *
 *	Dump out the pdev trace.
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
Fs_PdevPrintTrace(numRecs)
    int numRecs;
{
    if (numRecs < 0) {
	numRecs = pdevTraceLength;
    }
    printf("PDEV TRACE\n");
    (void)Trace_Print(pdevTraceHdrPtr, numRecs, Fs_PdevPrintRec);
}
