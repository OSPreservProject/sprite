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

#define MACH_UNIX_COMPAT

#include <sprite.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <mach.h>
#include <fs.h>
#include <fsutil.h>
#include <fsNameOps.h>
#include <fsUnixStubs.h>
#include <procUnixStubs.h>
#include <fsio.h>
#include <sync.h>
#include <list.h>
#include <proc.h>
#include <sig.h>
#include <dbg.h>
#include <timer.h>
#include <rpc.h>
#include <vm.h>

static char *errs[] = {"ENOERR", "EPERM", "ENOENT", "ESRCH", "EINTR", "EIO",
        "ENXIO", "E2BIG", "ENOEXEC", "EBADF", "ECHILD", "EAGAIN", "ENOMEM",
        "EACCES", "EFAULT", "ENOTBLK", "EBUSY", "EEXIST", "EXDEV", "ENODEV",
        "ENOTDIR", "EISDIR", "EINVAL", "ENFILE", "EMFILE", "ENOTTY",
        "ETXTBSY", "EFBIG", "ENOSPC", "ESPIPE", "EROFS", "EMLINK", "EPIPE",
        "EDOM", "ERANGE", "EWOULDBLOCK", "EINPROGRESS", "EALREADY", "ENOTSOCK",
        "EDESTADDRREQ", "EMSGSIZE", "EPROTOTYPE", "ENOPROTOOPT",
        "EPROTONOSUPPORT", "ESOCKTNOSUPPORT", "EOPNOTSUPP", "EPFNOSUPPORT",
        "EAFNOSUPPORT", "EADDRINUSE", "EADDRNOTAVAIL", "ENETDOWN",
        "ENETUNREACH", "ENETRESET", "ECONNABORTED", "ECONNRESET", "ENOBUFS",
        "EISCONN", "ENOTCONN", "ESHUTDOWN", "ETIMEDOUT", "ECONNREFUSED",
        "ELOOP", "ENAMETOOLONG", "EHOSTDOWN", "EHOSTUNREACH", "ENOTEMPTY",
        "EPROCLIM", "EUSERS", "EDQUOT", "ESTALE", "EREMOTE"};

#undef Mach_SetErrno
#define Mach_SetErrno(err) if (debugFsStubs) \
        printf("Error %d (%s) at %d in %s\n", err,\
        err<sizeof(errs)/sizeof(char *)?errs[err]:"",\
        __LINE__, __FILE__); Proc_GetActualProc()->unixErrno = (err)

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
 * Structure passed to the timeout proc to allow the process to be woken up.
 */
typedef struct {
    Proc_ControlBlock	*procPtr;
    int			timeOut;
} WakeupInfo;

/*
 * Routine called in FsSelect if the call timed-out.
 */
static void TimeoutProc _ARGS_((Timer_Ticks ticks, ClientData clientData));

static ReturnStatus readInMasks _ARGS_ ((int numStreams, int *userReadMaskPtr,
    int *readMaskPtr, int *userWriteMaskPtr, int *writeMaskPtr,
    int *userExceptMaskPtr, int *exceptMaskPtr));

static ReturnStatus writeOutMasks _ARGS_ ((int numStreams,
    int *userReadMaskPtr, int *readMaskPtr, int *userWriteMaskPtr, 
    int *writeMaskPtr, int *userExceptMaskPtr, int *exceptMaskPtr));

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
    Time		timeout;	/* Copy of *userTimeoutPtr. */
    Time                *timeoutPtr;
    int                 numReady = 0;
    int                 doTimeout;
    ReturnStatus        status, writeStatus;
    int			inReadMasks[MAX_NUM_ROWS];
    int			inWriteMasks[MAX_NUM_ROWS];
    int			inExceptMasks[MAX_NUM_ROWS];
    int			outReadMasks[MAX_NUM_ROWS];
    int			outWriteMasks[MAX_NUM_ROWS];
    int			outExceptMasks[MAX_NUM_ROWS];
    int                 *inReadMaskPtr;
    int                 *outReadMaskPtr;
    int                 *inWriteMaskPtr;
    int                 *outWriteMaskPtr;
    int                 *inExceptMaskPtr;
    int                 *outExceptMaskPtr;

    if ((userReadMaskPtr == (int *) USER_NIL) &&
        (userWriteMaskPtr == (int *) USER_NIL) &&
        (userExceptMaskPtr == (int *) USER_NIL)) {
	numStreams = 0;
    }

    /*
     * Make sure the number of streams is in the proper range.
     */
    if (numStreams < 0) {
	return(SYS_INVALID_ARG);
    } else if (numStreams > MAX_NUM_STREAMS) {
	numStreams = MAX_NUM_STREAMS;
    }

    if (userTimeoutPtr == (Time *) USER_NIL) {
	timeoutPtr = (Time *) NIL;
    } else {
	if (Vm_CopyIn(sizeof(Time), (Address) userTimeoutPtr, 
				(Address) &timeout) != SUCCESS) {
	    return(SYS_ARG_NOACCESS);
	}
	timeoutPtr = &timeout;
    }
    if (userReadMaskPtr == USER_NIL) {
	inReadMaskPtr = (int *) NIL;
	outReadMaskPtr = (int *) NIL;
    } else {
	inReadMaskPtr = inReadMasks;
	outReadMaskPtr = outReadMasks;
    }
    if (userWriteMaskPtr == USER_NIL) {
	inWriteMaskPtr = (int *) NIL;
	outWriteMaskPtr = (int *) NIL;
    } else {
	inWriteMaskPtr = inWriteMasks;
	outWriteMaskPtr = outWriteMasks;
    }
    if (userExceptMaskPtr == USER_NIL) {
	inExceptMaskPtr = (int *) NIL;
	outExceptMaskPtr = (int *) NIL;
    } else {
	inExceptMaskPtr = inExceptMasks;
	outExceptMaskPtr = outExceptMasks;
    }
    status = readInMasks(numStreams, userReadMaskPtr, inReadMaskPtr,
	userWriteMaskPtr, inWriteMaskPtr, userExceptMaskPtr, inExceptMaskPtr);
    if (status != SUCCESS) {
	return(SYS_ARG_NOACCESS);
    }
    status = Fs_Select(numStreams, timeoutPtr, inReadMaskPtr, outReadMaskPtr,
	inWriteMaskPtr, outWriteMaskPtr, inExceptMaskPtr, outExceptMaskPtr,
	&numReady, &doTimeout);
    if (status == SUCCESS || status == FS_TIMEOUT) {
	writeStatus = writeOutMasks(numStreams, userReadMaskPtr,
		outReadMaskPtr, userWriteMaskPtr, outWriteMaskPtr,
	       userExceptMaskPtr, outExceptMaskPtr);
	if (status == SUCCESS && doTimeout && writeStatus==SUCCESS) {
	    writeStatus = Vm_CopyOut(sizeof(timeout), (Address) &timeout, 
	                        (Address) userTimeoutPtr);
	}
	if (writeStatus != SUCCESS) {
	    status = SYS_ARG_NOACCESS;
	}
    }
    if (Vm_CopyOut(sizeof(*numReadyPtr), (Address) &numReady, 
                   (Address) numReadyPtr) != SUCCESS) {
	status = SYS_ARG_NOACCESS;
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_NewSelectStub --
 *
 *      The stub for the "select" Unix system call.
 *
 * Results:
 *      Returns -1 on failure.
 *
 * Side effects:
 *      Side effects associated with the system call.
 *
 *
 *----------------------------------------------------------------------
 */
int
Fs_NewSelectStub(numStreams, userReadMaskPtr, userWriteMaskPtr,
              userExceptMaskPtr, userTimeoutPtr)
    int		numStreams;	/* The length in bits of the read and write 
				 * masks. */
    int		*userReadMaskPtr;
				/* A bitmask indicating stream ID's to check
				 * for readability. (in/out) */
    int		*userWriteMaskPtr;
				/* A bitmask indicating stream ID's to check
				 * for writability. (in/out) */
    int		*userExceptMaskPtr;
				/* A bitmask indicating stream ID's to check
				 * for exception conditions. (in/out) */
    Time	*userTimeoutPtr;/* Timer value indicating timeout period or
				 * USER_NIL if no timeout. (in/out) */

{
    Time		timeout;	/* Copy of *userTimeoutPtr. */
    Time                *timeoutPtr;
    int                 numReady = 0;
    int                 doTimeout;
    ReturnStatus        status, writeStatus;
    int			inReadMasks[MAX_NUM_ROWS];
    int			inWriteMasks[MAX_NUM_ROWS];
    int			inExceptMasks[MAX_NUM_ROWS];
    int			outReadMasks[MAX_NUM_ROWS];
    int			outWriteMasks[MAX_NUM_ROWS];
    int			outExceptMasks[MAX_NUM_ROWS];
    int                 *inReadMaskPtr;
    int                 *outReadMaskPtr;
    int                 *inWriteMaskPtr;
    int                 *outWriteMaskPtr;
    int                 *inExceptMaskPtr;
    int                 *outExceptMaskPtr;
    extern int          debugFsStubs;

    if (debugFsStubs) {
	printf("Fs_NewSelectStub(%d, %x, %x, %x, %x)\n", numStreams,
		userReadMaskPtr, userWriteMaskPtr, userExceptMaskPtr,
		userTimeoutPtr);
    }

    if ((userReadMaskPtr == (int *) USER_NIL) &&
        (userWriteMaskPtr == (int *) USER_NIL) &&
        (userExceptMaskPtr == (int *) USER_NIL)) {
	numStreams = 0;
    }

    /*
     * Make sure the number of streams is in the proper range.
     */
    if (numStreams < 0) {
	Mach_SetErrno(EINVAL);
    } else if (numStreams > MAX_NUM_STREAMS) {
	numStreams = MAX_NUM_STREAMS;
    }

    if (userTimeoutPtr == (Time *) USER_NIL) {
	timeoutPtr = (Time *) NIL;
    } else {
	if (Vm_CopyIn(sizeof(Time), (Address) userTimeoutPtr, 
				(Address) &timeout) != SUCCESS) {
	    Mach_SetErrno(EFAULT);
	    return -1;
	}
	timeoutPtr = &timeout;
    }
    if (userReadMaskPtr == USER_NIL) {
	inReadMaskPtr = (int *) NIL;
	outReadMaskPtr = (int *) NIL;
    } else {
	inReadMaskPtr = inReadMasks;
	outReadMaskPtr = outReadMasks;
    }
    if (userWriteMaskPtr == USER_NIL) {
	inWriteMaskPtr = (int *) NIL;
	outWriteMaskPtr = (int *) NIL;
    } else {
	inWriteMaskPtr = inWriteMasks;
	outWriteMaskPtr = outWriteMasks;
    }
    if (userExceptMaskPtr == USER_NIL) {
	inExceptMaskPtr = (int *) NIL;
	outExceptMaskPtr = (int *) NIL;
    } else {
	inExceptMaskPtr = inExceptMasks;
	outExceptMaskPtr = outExceptMasks;
    }

    status = readInMasks(numStreams, userReadMaskPtr, inReadMaskPtr,
	userWriteMaskPtr, inWriteMaskPtr, userExceptMaskPtr, inExceptMaskPtr);
    if (status != SUCCESS) {
	Mach_SetErrno(EFAULT);
	return -1;
    }
    status = Fs_Select(numStreams, timeoutPtr, inReadMaskPtr, outReadMaskPtr,
	inWriteMaskPtr, outWriteMaskPtr, inExceptMaskPtr, outExceptMaskPtr,
	&numReady, &doTimeout);
    if (status == SUCCESS || status == FS_TIMEOUT) {
	writeStatus = writeOutMasks(numStreams, userReadMaskPtr,
		outReadMaskPtr, userWriteMaskPtr, outWriteMaskPtr,
	        userExceptMaskPtr, outExceptMaskPtr);
	if (status == SUCCESS && doTimeout && writeStatus==SUCCESS) {
	    writeStatus = Vm_CopyOut(sizeof(timeout), (Address) &timeout, 
	                        (Address) userTimeoutPtr);
	}
	if (writeStatus != SUCCESS) {
	    status = SYS_ARG_NOACCESS;
	}
    }
    if (status == SUCCESS) {
	return numReady;
    } else if (status == GEN_ABORTED_BY_SIGNAL) {
	Proc_GetCurrentProc()->unixProgress = PROC_PROGRESS_MIG_RESTART;
	Mach_SetErrno(EINTR);
	return -1;
    } else if (status == FS_TIMEOUT) {
	return 0;
    } else {
	Mach_SetErrno(EACCES);
	return -1;
    }
}

/*ARGSUSED*/
ReturnStatus
Fs_Select(numStreams, timeoutPtr, inReadMaskPtr, outReadMaskPtr,
    inWriteMaskPtr, outWriteMaskPtr, inExceptMaskPtr, outExceptMaskPtr,
    numReadyPtr, doTimeoutPtr)
    int		numStreams;	/* The length in bits of the read and write 
				 * masks. */
    Time	*timeoutPtr;    /* Timer value indicating timeout period or
				 * NIL if no timeout. (in/out) */
    int		*inReadMaskPtr;
    int		*outReadMaskPtr;
				/* A bitmask indicating stream ID's to check
				 * for readability. (in/out) */
    int		*inWriteMaskPtr;
    int		*outWriteMaskPtr;
				/* A bitmask indicating stream ID's to check
				 * for writability. (in/out) */
    int		*inExceptMaskPtr;
    int		*outExceptMaskPtr;
				/* A bitmask indicating stream ID's to check
				 * for exception conditions. (in/out) */
    int		*numReadyPtr;	/* On return indicates the number of streams
				 * ready for I/O. (out) */
    int         *doTimeoutPtr;				 

{
    Proc_ControlBlock	*procPtr;	/* This proc's control block */
    Timer_QueueElement	wakeupElement;	/* Element for timeout. */
    WakeupInfo		wakeupInfo;	/* Passed to timeout routine. */
    Sync_RemoteWaiter	waiter;
    int			row;		/* Index of row of inReadMasks,
					 * inWriteMasks, inExceptMasks. */
    register int	mask;		/* Selects bit within a row of
					 * inReadMasks, inWriteMasks,
					 * inExceptMasks. */
    register int	inReadMask = 0;	/* Contents of a row of inReadMasks. */
    register int	inWriteMask = 0;/* Contents of a row of inWriteMasks. */
    int			inExceptMask = 0;/* Content of a row of inExceptMasks.*/
    int			intsInMask;	/* # of integers in inReadMasks,
					 * inWriteMasks and inExceptMasks. */
    register int	bit;		/* Loop counter */
    int			bitMax;		/* Loop terminating condition */
    Boolean		poll;		/* If TRUE, don't wait if the first
					 * check of streams finds that none
					 * are ready now. */
    int			s;		/* Temp copy of numStreams */
    ReturnStatus	status = SUCCESS;

    /*
     * If all the masks are NIL, then there aren't any streams to select.
     * Set the numStreams to zero so we can see if we can return once the
     * timeout argument is examined.
     */
    if ((inReadMaskPtr == (int *) NIL) &&
        (inWriteMaskPtr == (int *) NIL) &&
        (inExceptMaskPtr == (int *) NIL)) {
	numStreams = 0;
    }

    /*
     * See if a timeout period was given. If so, set up a timer
     * queue element to call TimeoutProc to wakeup the process.
     * If the timeout is 0 or negative, just poll the streams to
     * see if any are ready.
     */

    if (timeoutPtr == (Time *) NIL) {
	poll = FALSE;
	*doTimeoutPtr = FALSE;
    } else {
	if ((timeoutPtr->seconds < 0) || 
	    ((timeoutPtr->seconds == 0) && (timeoutPtr->microseconds == 0))) {

	    /*
	     * A zero or negative time was given. Assume the user wants to
	     * poll the streams.
	     */
	    *doTimeoutPtr = FALSE;
	    poll = TRUE;

	} else if (numStreams == 0) {

	    /*
	     * Special case: nothing to select, but a valid timeout period
	     * was specified. Just wait for the timeout to expire.
	     */
	    if (Sync_WaitTime(*timeoutPtr)) {
		return GEN_ABORTED_BY_SIGNAL;
	    } else {
		return FS_TIMEOUT;
	    }
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
	    Timer_TimeToTicks(*timeoutPtr, &ticks);
	    Timer_GetCurrentTicks(&currentTicks);
	    Timer_AddTicks(currentTicks, ticks, &(wakeupElement.time));
	    poll = FALSE;
	    *doTimeoutPtr = TRUE;
	}
    }

    /*
     * Nothing to select and no timeout specified so just return.
     */
    if (numStreams == 0) {
	return status;
    }

    intsInMask = (numStreams + (BITS_PER_ROW -1)) / BITS_PER_ROW;
    procPtr = Proc_GetCurrentProc();

    /*
     * If a timeout period was specified, set up a callback from the Timer 
     * queue.
     */
    wakeupInfo.timeOut = FALSE;
    if (*doTimeoutPtr) {
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

	    if (inReadMaskPtr != (int *) NIL) {
		inReadMask = inReadMaskPtr[row];
	    }
	    if (inWriteMaskPtr != (int *) NIL) {
		inWriteMask = inWriteMaskPtr[row];
	    }
	    if (inExceptMaskPtr != (int *) NIL) {
		inExceptMask = inExceptMaskPtr[row];
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

			if (Fs_GetStreamPtr(procPtr, row * BITS_PER_ROW + bit, 
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

			    assert(((int) streamPtr & 3) == 0);
			    assert(((int) streamPtr->ioHandlePtr & 3) == 0);

			    status = 
	    (*fsio_StreamOpTable[streamPtr->ioHandlePtr->fileID.type].select)
				(streamPtr->ioHandlePtr, &waiter,
				 &readBit, &writeBit, &exceptBit);
			    if (status != SUCCESS) {
				goto deschedule;
			    }
			    if (readBit | writeBit | exceptBit) {
				outReadMask |= readBit & mask;
				outWriteMask |= writeBit & mask;
				outExceptMask |= exceptBit & mask;
				++*numReadyPtr;
			    }
			}
		    }
		}
		s -= BITS_PER_ROW;
	    }
	    if (outReadMaskPtr != (int *) NIL) {
		outReadMaskPtr[row]   = outReadMask;
	    }
	    if (outWriteMaskPtr != (int *) NIL) {
		outWriteMaskPtr[row]  = outWriteMask;
	    }
	    if (outExceptMaskPtr != (int *) NIL) {
		outExceptMaskPtr[row] = outExceptMask;
	    }
	}

	/*
	 * If at least 1 stream is ready or we're just polling, then quit.
	 * Otherwise, wait until we're notified that some stream became
	 * ready. When we wake up, start the loop again to find out which 
	 * stream(s) became ready.
	 */

	if (*numReadyPtr > 0 || poll) {
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
    if (!wakeupInfo.timeOut && *doTimeoutPtr) {
	Timer_DescheduleRoutine(&wakeupElement);
    }

    /*
     * Only copy out the masks if something is ready,
     * or upon a timeout.  (Emacs, in particular, stupidly looks
     * at the read masks after a timeout.).
     */
    if (status == SUCCESS || status == FS_TIMEOUT) {
	if (status != SUCCESS) {
	    for (row=0 ; row<intsInMask ; row++) {
		if (outReadMaskPtr != (int *) NIL) {
		    outReadMaskPtr[row] = 0;
		}
		if (outWriteMaskPtr != (int *) NIL) {
		    outWriteMaskPtr[row] = 0;
		}
		if (outExceptMaskPtr != (int *) NIL) {
		    outExceptMaskPtr[row] = 0;
		}
	    }
	}
	if (status == SUCCESS && *doTimeoutPtr) {
	    /*
	     * A timeout period was given but some stream became ready
	     * before the period expired.  Return the amount of time that
	     * is remaining in the timeout value.
	     */

	    Timer_Ticks temp;

	    Timer_GetCurrentTicks(&temp);
	    Timer_SubtractTicks(wakeupElement.time, temp, &temp);
	    Timer_TicksToTime(temp, timeoutPtr);
	}
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
TimeoutProc(ticks, clientData)
    Timer_Ticks	ticks;
    ClientData	clientData;
{
    WakeupInfo	*wakeupInfoPtr = (WakeupInfo *) clientData;
    wakeupInfoPtr->timeOut = TRUE;
    Sync_ProcWakeup(wakeupInfoPtr->procPtr->processID,
                    wakeupInfoPtr->procPtr->waitToken);
}

static ReturnStatus
readInMasks(numStreams, userReadMaskPtr, readMaskPtr,
    userWriteMaskPtr, writeMaskPtr, userExceptMaskPtr, exceptMaskPtr)
    int         numStreams;
    int		*userReadMaskPtr;
    int		*readMaskPtr;
    int		*userWriteMaskPtr;
    int		*writeMaskPtr;
    int		*userExceptMaskPtr;
    int		*exceptMaskPtr;
{
    int bytesInMask;
    int status;

    if (numStreams == 0) {
	return SUCCESS;
    }
    bytesInMask = ((numStreams+(BITS_PER_ROW -1))/BITS_PER_ROW)*sizeof(int);

    /*
     * Copy in the masks from user's address space.
     */
    if (userReadMaskPtr != (int *) USER_NIL) {
	status = Vm_CopyIn(bytesInMask, (Address) userReadMaskPtr, 
			   (Address) readMaskPtr);
	if (status != SUCCESS) {
	    return(status);
	}
    }
    if (userWriteMaskPtr != (int *) USER_NIL) {
	status = Vm_CopyIn(bytesInMask, (Address) userWriteMaskPtr, 
				(Address) writeMaskPtr);
	if (status != SUCCESS) {
	    return(status);
	}
    }
    if (userExceptMaskPtr != (int *) USER_NIL) {
	status = Vm_CopyIn(bytesInMask, (Address) userExceptMaskPtr, 
				(Address) exceptMaskPtr);
	if (status != SUCCESS) {
	    return(status);
	}
    }
    return SUCCESS;
}

static ReturnStatus
writeOutMasks(numStreams, userReadMaskPtr, readMaskPtr,
    userWriteMaskPtr, writeMaskPtr, userExceptMaskPtr, exceptMaskPtr)
    int         numStreams;
    int		*userReadMaskPtr;
    int		*readMaskPtr;
    int		*userWriteMaskPtr;
    int		*writeMaskPtr;
    int		*userExceptMaskPtr;
    int		*exceptMaskPtr;
{
    int bytesInMask;
    int status;

    if (numStreams == 0) {
	return SUCCESS;
    }
    bytesInMask = ((numStreams+(BITS_PER_ROW -1))/BITS_PER_ROW)*sizeof(int);
    if (userReadMaskPtr != (int *)USER_NIL) {
	status = Vm_CopyOut(bytesInMask, (Address) readMaskPtr,
	                      (Address) userReadMaskPtr);
	if (status != SUCCESS) {
	    return(SYS_ARG_NOACCESS);
	}
    }
    if (userWriteMaskPtr != (int *) USER_NIL) {
	status = Vm_CopyOut(bytesInMask, (Address) writeMaskPtr,
	                      (Address) userWriteMaskPtr);
	if (status != SUCCESS) {
	    return(SYS_ARG_NOACCESS);
	}
    }
    if (userExceptMaskPtr != (int *) USER_NIL) {
	status = Vm_CopyOut(bytesInMask, (Address) exceptMaskPtr,
	                      (Address) userExceptMaskPtr);
	if (status != SUCCESS) {
	    return(SYS_ARG_NOACCESS);
	}
    }
    return SUCCESS;
}

