/* 
 * fsSelect.c --
 *
 *	Routines to implement the Fs_Select system call.
 *
 *	Features:
 *	1) There is a limit on the number of streams that can be selected.
 *	   (This routine silently limits it to 1024.)
 *	2) If the 3 bit masks are NULL, and the timeout value is not 0, 
 *	   then Sync_WaitTime is called to wait and FS_TIMEOUT is returned
 *	   with numReady = 0.
 *	3) The file-type select routine must handle an empty inFlags value
 *	   by setting outFlags to 0.
 *	4) If all bits are cleared in the bit masks, SUCCESS is returned
 *	   with numReady = 0.

 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "fs.h"
#include "fsInt.h"
#include "fsOpTable.h"
#include "sync.h"
#include "list.h"
#include "proc.h"
#include "sig.h"
#include "dbg.h"
#include "timer.h"
#include "rpc.h"


/*
 * Internal limit on the number of streams that can be checked.
 * This needs to be moved to an external header file!!
 */
#define MAX_NUM_STREAMS		1024

/*
 * Number of bits within a row of the bitmask. Assumes a row
 * is a 32-bit integer.
 */
#define BITS_PER_ROW	32

/*
 * Maximum number of rows of bitmasks.
 */
#define MAX_NUM_ROWS	(MAX_NUM_STREAMS / BITS_PER_ROW)

/*
 * Routine called in FsSelect if the call timed-out.
 */
static void TimeoutProc();

/*
 * Structure passed to the timeout proc to allow the process to be woken up.
 */
typedef struct {
    Proc_ControlBlock	*procPtr;
    int			timeOut;
} WakeupInfo;



/*
 *----------------------------------------------------------------------
 *
 * Fs_SelectStub --
 *
 *      This is the stub for the Fs_Select system call. The bitmasks
 *	are examined to see if the corresponding stream is readble, writable,
 *	and/or has an exception condition pending. The user may give a
 *	timeout period to limit the amount of time to wait.
 *
 * Results:
 *	SUCCESS			- the operation was successful.
 *	FS_TIMEOUT		- if a timeout period was specified, and no
 *			  	  streams were ready within the time-out period.
 *	SYS_ARG_NOACCESS	- an invalid address for an argument was
 *				  given.
 *	SYS_INVALID_ARG		- an invalid stream ID was given in one
 *				  of the bitmaps.
 *	GEN_ABORTED_BY_SIGNAL	- a signal came in.
 *
 * Side effects:
 *	The process may be put to sleep.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
ReturnStatus
Fs_SelectStub(numStreams, userTimeoutPtr, userReadMaskPtr, userWriteMaskPtr, 
	userExceptMaskPtr, numReadyPtr)
    int		numStreams;	/* The length in bits of the read and write 
				 * masks. */
    Time	*userTimeoutPtr;/* Timer value indicating timeout period or
				 * USER_NIL if no timeout. (in/out) */
    int		*userReadMaskPtr;
				/* A bitmask indicating stream ID's to check
				 * for readability. (in/out) */
    int		*userWriteMaskPtr;
				/* A bitmask indicating stream ID's to check
				 * for writability. (in/out) */
    int		*userExceptMaskPtr;
				/* A bitmask indicating stream ID's to check
				 * for exception conditions. (in/out) */
    int		*numReadyPtr;	/* On return indicates the number of streams
				 * ready for I/O. (out) */
				   
{
    Proc_ControlBlock	*procPtr;	/* This proc's control block */
    Timer_QueueElement	wakeupElement;	/* Element for timeout. */
    WakeupInfo		wakeupInfo;	/* Passed to timeout routine. */
    Time		timeout;	/* Copy of *userTimeoutPtr. */
    Sync_RemoteWaiter	waiter;
    int			row;		/* Index of row of inReadMasks,
					 * inWriteMasks, inExceptMasks. */
    register int	mask;		/* Selects bit within a row of
					 * inReadMasks, inWriteMasks,
					 * inExceptMasks. */
    register int	inReadMask;	/* Contents of a row of inReadMasks. */
    register int	inWriteMask;	/* Contents of a row of inWriteMasks. */
    int			inExceptMask;	/* Contents of a row of inExceptMasks.*/
    int			intsInMask;	/* # of integers in inReadMasks,
					 * inWriteMasks and inExceptMasks. */
    int			bytesInMask;	/* # of bytes in inReadMasks,
					 * outWriteMasks and outExceptMasks. */
    register int	bit;		/* Loop counter */
    int			bitMax;		/* Loop terminating condition */
    int			inReadMasks[MAX_NUM_ROWS];
    int			inWriteMasks[MAX_NUM_ROWS];
    int			inExceptMasks[MAX_NUM_ROWS];
    int			outReadMasks[MAX_NUM_ROWS];
    int			outWriteMasks[MAX_NUM_ROWS];
    int			outExceptMasks[MAX_NUM_ROWS];

    Boolean		doTimeout;	/* TRUE if valid timeout given. */
    Boolean		poll;		/* If TRUE, don't wait if the first
					 * check of streams finds that none
					 * are ready now. */
    int			numReady = 0;	/* Number of ready streams. If a stream
					 * is readable and/or writable and/or
					 * has a exception pending, it is
					 * only counted once. */
    int			s;		/* Temp copy of numStreams */
    ReturnStatus	status = SUCCESS;


    /*
     * Make sure the number of streams is in the proper range.
     */
    if (numStreams < 0) {
	return(SYS_INVALID_ARG);
    } else if (numStreams > MAX_NUM_STREAMS) {
	numStreams = MAX_NUM_STREAMS;
    }

    /*
     * If all the masks are USER_NIL, then there aren't any streams to select.
     * Set the numStreams to zero so we can see if we can return once the
     * timeout argument is examined.
     */
    if ((userReadMaskPtr == (int *) USER_NIL) &&
        (userWriteMaskPtr == (int *) USER_NIL) &&
        (userExceptMaskPtr == (int *) USER_NIL)) {
	numStreams = 0;
    }

    /*
     * See if a timeout period was given. If so, set up a timer
     * queue element to call TimeoutProc to wakeup the process.
     * If the timeout is 0 or negative, just poll the streams to
     * see if any are ready.
     */

    if (userTimeoutPtr == (Time *) USER_NIL) {
	poll = FALSE;
	doTimeout = FALSE;
    } else {

	if (Vm_CopyIn(sizeof(Time), (Address) userTimeoutPtr, 
				(Address) &timeout) != SUCCESS) {
	    return(SYS_ARG_NOACCESS);
	}

	if ((timeout.seconds < 0) || 
	    ((timeout.seconds == 0) && (timeout.microseconds == 0))) {

	    /*
	     * A zero or negative time was given. Assume the user wants to
	     * poll the streams.
	     */
	    doTimeout = FALSE;
	    poll = TRUE;

	} else if (numStreams == 0) {

	    /*
	     * Special case: nothing to select, but a valid timeout period
	     * was specified. Just wait for the timeout to expire.
	     */
	    if (Sync_WaitTime(timeout)) {
		status = GEN_ABORTED_BY_SIGNAL;
	    } else {
		status = FS_TIMEOUT;
	    }
	    goto allDone;

	} else {
	    Timer_Ticks ticks;
	    Timer_Ticks currentTicks;
	    wakeupElement.routine = TimeoutProc;

	    /*
	     * Convert the user's timeout value from a relative Time to a 
	     * an absolute time in the internal Timer_Ticks units.
	     *
	     * The value wakeupElement.time is used at the end of this
	     * routine to return the amount of time remaining in the timeout.
	     */
	    Timer_TimeToTicks(timeout, &ticks);
	    Timer_GetCurrentTicks(&currentTicks);
	    Timer_AddTicks(currentTicks, ticks, &(wakeupElement.time));
	    poll = FALSE;
	    doTimeout = TRUE;
	}
    }

    /*
     * Nothing to select and no timeout specified so just return.
     */
    if (numStreams == 0) {
	goto allDone;
    }


    intsInMask = (numStreams + (BITS_PER_ROW -1)) / BITS_PER_ROW;
    bytesInMask = intsInMask * sizeof(inReadMask);

    /*
     * Copy in the masks from user's address space.
     */
    if (userReadMaskPtr != (int *) USER_NIL) {
	status = Vm_CopyIn(bytesInMask, (Address) userReadMaskPtr, 
			   (Address) inReadMasks);
	if (status != SUCCESS) {
	    return(status);
	}
    } else {
	inReadMask = 0;
    }

    if (userWriteMaskPtr != (int *) USER_NIL) {
	status = Vm_CopyIn(bytesInMask, (Address) userWriteMaskPtr, 
				(Address) inWriteMasks);
	if (status != SUCCESS) {
	    return(status);
	}
    } else {
	inWriteMask = 0;
    }

    if (userExceptMaskPtr != (int *) USER_NIL) {
	status = Vm_CopyIn(bytesInMask, (Address) userExceptMaskPtr, 
				(Address) inExceptMasks);
	if (status != SUCCESS) {
	    return(status);
	}
    } else {
	inExceptMask = 0;
    }

    procPtr = Proc_GetCurrentProc();

    /*
     * If a timeout period was specified, set up a callback from the Timer 
     * queue.
     */
    wakeupInfo.timeOut = FALSE;
    if (doTimeout) {
	wakeupElement.clientData = (ClientData) &wakeupInfo;
	wakeupInfo.procPtr = procPtr;
	Timer_ScheduleRoutine(&wakeupElement, FALSE);
    }

    waiter.hostID = rpc_SpriteID;

    while (TRUE) {

	/*
	 * Get the token to use for select waiting.  We must get the token
	 * before checking the timeout flag because there is a race 
	 * condition between the timeout wakeup and us getting the
	 * wait token.  If we checked timeOut before getting the token, it 
	 * is possible that a time out could come between checking the 
	 * flag and getting the wait token and we could miss the time out.
	 */
	Sync_GetWaitToken(&waiter.pid, &waiter.waitToken);
	if (wakeupInfo.timeOut) {
	    status = FS_TIMEOUT;
	    break;
	}

	/*
	 * The read, write and except bit masks can be considered as
	 * arrays of bits, possibly more than 32 bits long. Each mask is
	 * represented as an array of ints such that row 0 corresponds to
	 * streams 0 through 31, row 1 corresponds to streams 32 though
	 * 63, etc. Within a row, the low-order bit corresponds to the
	 * smallest stream number.
	 */
	s = numStreams + 1;
	for (row = 0; row < intsInMask; row++) {
	    int	outReadMask = 0;
	    int	outWriteMask = 0;
	    int	outExceptMask = 0;

	    if (userReadMaskPtr != (int *) USER_NIL) {
		inReadMask = inReadMasks[row];
	    }
	    if (userWriteMaskPtr != (int *) USER_NIL) {
		inWriteMask = inWriteMasks[row];
	    }
	    if (userExceptMaskPtr != (int *) USER_NIL) {
		inExceptMask = inExceptMasks[row];
	    }
	    if (inReadMask != 0 || inWriteMask != 0 || inExceptMask != 0) {
		/*
		 * At least one stream in this row was selected. Go through
		 * the masks to find the stream number and see if it's ready.
		 */
		bitMax = (s > BITS_PER_ROW) ? BITS_PER_ROW : s;
		for (mask = 1, bit = 0; bit < bitMax; mask <<= 1, bit++) {
		    /*
		     * Set up single bit masks that will be or'ed into
		     * the final result masks.
		     */
		    int readBit = inReadMask & mask;
		    int writeBit = inWriteMask & mask;
		    int exceptBit = inExceptMask & mask;

		    if (readBit | writeBit | exceptBit) {
			Fs_Stream	*streamPtr;

			if (FsGetStreamPtr(procPtr, row * BITS_PER_ROW + bit, 
						    &streamPtr) != SUCCESS) {
			    /*
			     *  A stream was selected that probably 
			     *  wasn't opened.
			     */
			    status = SYS_INVALID_ARG;
			    goto deschedule;
			} else {
			    if (!(streamPtr->flags & FS_READ)) {
				readBit = 0;
			    }
			    if (!(streamPtr->flags & FS_WRITE)) {
				writeBit = 0;
			    }
			    /*
			     * Call the I/O handle's select routine and
			     * combine what's left in the single bit masks
			     * into the final result masks.
			     */
			    status = 
		(*fsStreamOpTable[streamPtr->ioHandlePtr->fileID.type].select)
				(streamPtr->ioHandlePtr, &waiter,
				 &readBit, &writeBit, &exceptBit);
			    if (status != SUCCESS) {
				goto deschedule;
			    }
			    if (readBit | writeBit | exceptBit) {
				outReadMask |= readBit & mask;
				outWriteMask |= writeBit & mask;
				outExceptMask |= exceptBit & mask;
				numReady++;
			    }
			}
		    }
		}
		s -= BITS_PER_ROW;
	    }
	    outReadMasks[row]   = outReadMask;
	    outWriteMasks[row]  = outWriteMask;
	    outExceptMasks[row] = outExceptMask;
	}

	/*
	 * If at least 1 stream is ready or we're just polling, then quit.
	 * Otherwise, wait until we're notified that some stream became
	 * ready. When we wake up, start the loop again to find out which 
	 * stream(s) became ready.
	 */

	if (numReady > 0 || poll) {
	    break;
	} else {
	    if (Sync_ProcWait((Sync_Lock *) NIL, TRUE)) {
		status = GEN_ABORTED_BY_SIGNAL;
		break;
	    }
	}
    }

    /*
     * The wakeupInfo.timedOut flag is set by the routine called from 
     * the timer queue. If the flag is not set, then remove the routine 
     * from the queue.
     */
deschedule:
    if (!wakeupInfo.timeOut && doTimeout) {
	Timer_DescheduleRoutine(&wakeupElement);
    }
    
    /*
     * Only copy out the masks if there aren't any errors.
     */
    if (status == SUCCESS) {

	if (userReadMaskPtr != (int *)USER_NIL) {
	    status = Vm_CopyOut(bytesInMask, (Address) outReadMasks,
					(Address) userReadMaskPtr);
	    if (status != SUCCESS) {
		return(SYS_ARG_NOACCESS);
	    }
	}

	if (userWriteMaskPtr != (int *) USER_NIL) {
	    status = Vm_CopyOut(bytesInMask, (Address) outWriteMasks,
					(Address) userWriteMaskPtr);
	    if (status != SUCCESS) {
		return(SYS_ARG_NOACCESS);
	    }
	}

	if (userExceptMaskPtr != (int *) USER_NIL) {
	    status = Vm_CopyOut(bytesInMask, (Address) outExceptMasks,
					(Address) userExceptMaskPtr);
	    if (status != SUCCESS) {
		return(SYS_ARG_NOACCESS);
	    }
	}

	if (doTimeout) {
	    /*
	     * A timeout period was given but some stream became ready
	     * before the period expired.  Return the amount of time that
	     * is remaining in the timeout value.
	     */

	    Timer_Ticks temp;

	    Timer_GetCurrentTicks(&temp);
	    Timer_SubtractTicks(wakeupElement.time, temp, &temp);
	    Timer_TicksToTime(temp, &timeout);

	    status = Vm_CopyOut(sizeof(timeout), (Address) &timeout, 
					(Address) userTimeoutPtr);
	    if (status != SUCCESS) {
		return(SYS_ARG_NOACCESS);
	    }
	}
    }

allDone:

    /*
     * We arrive here if status is SUCCESS or FS_TIMEOUT. 
     * Attempt to get a good value of numReady out to the user process.
     */
    if (Vm_CopyOut(sizeof(*numReadyPtr), (Address) &numReady, 
	    (Address) numReadyPtr) != SUCCESS) {
	status = SYS_ARG_NOACCESS;
    }

    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * TimeoutProc --
 *
 *	This routine is called from the Timer queue if a select
 *	call does not complete by a certain time.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The timeOut field is set to TRUE. Other processes may be woken up.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
static void
TimeoutProc(ticks, wakeupInfoPtr)
    Timer_Ticks	ticks;
    WakeupInfo	*wakeupInfoPtr;
{
    wakeupInfoPtr->timeOut = TRUE;
    Sync_ProcWakeup(wakeupInfoPtr->procPtr->processID,
                    wakeupInfoPtr->procPtr->waitToken);
}
