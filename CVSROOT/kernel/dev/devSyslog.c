/* 
 * devSyslog.c --
 *
 *	Device driver for processing internal kernel writes to the console.
 *			
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "dev.h"
#include "fs.h"
#include "sys.h"
#include "machMon.h"
#include "sig.h"
#include "mem.h"
#include "sync.h"
#include "sched.h"
#include "proc.h"
#include "timer.h"
#include "dbg.h"
#include "varg.h"
#include "user/list.h"

/*
 * Definition of mutual exclusion variable.
 */
static int	syslogMutex = 0;
/*
 * Monitored circular buffer of data written out by the kernel.
 */
#define	SYSLOG_BUF_SIZE	8192
static	char	syslogBuffer[SYSLOG_BUF_SIZE];
static	int	firstIndex = -1;
static	int	lastIndex = -1;

static	Boolean	openForReading = FALSE;	/* TRUE if the device is open for
					 * reading. */
static	ClientData notifyToken;	/* Used for Fs call-back to notify waiting
				 * processes that the syslog device is ready.*/
static	Boolean	   overflow = FALSE;


/*
 *----------------------------------------------------------------------
 *
 * Dev_SyslogOpen --
 *
 *	Open the system log device.
 *
 * Results:
 *	SUCCESS		- the device was opened.
 *	FAILURE		- the event device is already opened.
 *
 * Side effects:
 *	The system log is "opened" and static variables are initialized.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Dev_SyslogOpen(devicePtr, useFlags, token)
    Fs_Device *devicePtr;	/* Specifies type and unit number. */
    int useFlags;		/* Flags from the stream.  We only allow
				 * a single reader at one time. */
    ClientData token;		/* Used for Fs call-back to notify waiting
				 * processes that the syslog device is ready.*/
{
    MASTER_LOCK(syslogMutex);
    if (useFlags & FS_READ) {
	if (openForReading) {
	    MASTER_UNLOCK(syslogMutex);
	    return(FAILURE);
	}
	openForReading = TRUE;
    }
    notifyToken = token;
    MASTER_UNLOCK(syslogMutex);
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Dev_SyslogRead --
 *
 *	Copy data out of the syslog buffer.
 *
 * Results:
 *	FS_WOULD_BLOCK if no data available, otherwise SUCCESS.
 *
 * Side effects:
 *	Data is removed from the syslog buffer.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Dev_SyslogRead(devicePtr, offset, bufSize, bufPtr, lenPtr)
    Fs_Device	*devicePtr;
    int		offset;	  	/* Ignored. */
    int		bufSize;	/* Size of buffer. */
    Address	bufPtr;		/* Place to store data. */
    register int *lenPtr;  	/* Maximum number of chars to read 
				 * before returning. */ 
{
    int		toRead;
    int		origSize;
    int		bytesLeft;
    Address	copyAddr;

    origSize = *lenPtr;
    bytesLeft = *lenPtr;

    MASTER_LOCK(syslogMutex);
    overflow = FALSE;
    if (firstIndex == -1) {
	*lenPtr = 0;
	MASTER_UNLOCK(syslogMutex);
	return(FS_WOULD_BLOCK);
    }
    if (firstIndex > lastIndex) {
	toRead = SYSLOG_BUF_SIZE - firstIndex;
	copyAddr = &syslogBuffer[firstIndex];
	if (toRead <= bytesLeft) {
	    firstIndex = 0;
	} else {
	    toRead = bytesLeft;
	    firstIndex += toRead;
	}
	bcopy(copyAddr, bufPtr,toRead);
	bufPtr += toRead;
	bytesLeft -= toRead;
    }
    if (bytesLeft > 0) {
	toRead = lastIndex - firstIndex + 1;
	copyAddr = &syslogBuffer[firstIndex];
	if (toRead <= bytesLeft) {
	    firstIndex = -1;
	    lastIndex = -1;
	} else {
	    toRead = bytesLeft;
	    firstIndex += toRead;
	}
	bcopy(copyAddr, bufPtr,toRead);
	bytesLeft -= toRead;
    }
    MASTER_UNLOCK(syslogMutex);
    *lenPtr = origSize - bytesLeft;
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Dev_SyslogWrite --
 *
 *	Write characters to the system log.  If the device isn't open
 *	then Mach_MonPutChar is used to output directly to the screen.  
 *	Otherwise data is output to a buffer.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	Data is written to the console or to the buffer.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Dev_SyslogWrite(devicePtr, offset, bufSize, bufPtr, bytesWrittenPtr)
    Fs_Device     *devicePtr;		/* Ignored. */
    int		 offset;		/* Ignored. */
    int		 bufSize;		/* Number of bytes to write. */
    register char *bufPtr;		/* Place to find data */
    int 	 *bytesWrittenPtr;	/* Number of chars written */ 
{
    int	origSize;
    int	i;
    int	toWrite;


    if (!dbg_UsingSyslog) {
	if (!openForReading) {
	    for (i = 0; i < bufSize; i++, bufPtr++) {
		Mach_MonPutChar(*bufPtr);
	    }
	    *bytesWrittenPtr = bufSize;
	    return(SUCCESS);
	}
	MASTER_LOCK(syslogMutex);
    }

    origSize = bufSize;
    if (firstIndex != -1 && firstIndex <= lastIndex) {
	/*
	 * Fill up until the end of the buffer or until run out of
	 * bytes, whichever comes first.
	 */
	toWrite = SYSLOG_BUF_SIZE - (lastIndex + 1);
	if (toWrite > bufSize) {
	    toWrite = bufSize;
	}
	bcopy(bufPtr, &syslogBuffer[lastIndex + 1], toWrite);
	bufSize -= toWrite;
	bufPtr += toWrite;
	lastIndex += toWrite;
    }
    if (bufSize > 0) {
	int	nextIndex;

	nextIndex = lastIndex + 1;
	if (nextIndex == SYSLOG_BUF_SIZE) {
	    nextIndex = 0;
	}
	if (nextIndex == firstIndex) {
	    /*
	     * Buffer overflow
	     */
	    if (!overflow) {
		Mach_MonPrintf("Dev_SyslogWrite: Buffer overflow, dumping overflow ...\n");
		overflow = TRUE;
	    }
	    for (i = 0; i < bufSize; i++, bufPtr++) {
		Mach_MonPutChar(*bufPtr);
	    }
	} else {
	    if (firstIndex == -1) {
		toWrite = SYSLOG_BUF_SIZE;
		firstIndex = 0;
	    } else {
		toWrite = firstIndex - nextIndex;
	    }
	    if (toWrite > bufSize) {
		toWrite = bufSize;
	    }
	    bcopy(bufPtr, syslogBuffer, toWrite);
	    lastIndex = nextIndex + toWrite - 1;
	    bufSize -= toWrite;
	}
    }
    if (!dbg_UsingSyslog) {
	Fs_NotifyReader(notifyToken);
	MASTER_UNLOCK(syslogMutex);
    }
    *bytesWrittenPtr = origSize - bufSize;
    return(SUCCESS);
}



/*
 *----------------------------------------------------------------------
 *
 * Dev_SyslogPutChar --
 *
 *	Write a single character to the system log.
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
Dev_SyslogPutChar(ch)
    char	ch;
{
    int	bytesWritten;

    (void) Dev_SyslogWrite((Fs_Device *) NIL, 0, 1, &ch, &bytesWritten);
}


/*
 *----------------------------------------------------------------------
 *
 * Dev_SyslogClose --
 *
 *	Close the system log and flush any remaining output.
 *
 * Results:
 *	SUCCESS		- always returned.
 *
 * Side effects:
 *	The console is "closed".
 *
 *----------------------------------------------------------------------
 */
/* ARGSUSED */
ReturnStatus
Dev_SyslogClose(devicePtr, useFlags, openCount, writerCount)
    Fs_Device	*devicePtr;
    int		useFlags;
    int		openCount;
    int		writerCount;
{
    int	i;

    MASTER_LOCK(syslogMutex);
    if (useFlags & FS_READ) {
	openForReading = FALSE;
	if (firstIndex != -1) {
	    if (firstIndex <= lastIndex) {
		for (i = firstIndex; i <= lastIndex; i++) {
		    Mach_MonPutChar(syslogBuffer[i]);
		}
	    } else {
		for (i = firstIndex; i < SYSLOG_BUF_SIZE; i++) {
		    Mach_MonPutChar(syslogBuffer[i]);
		}
		for (i = 0; i <= lastIndex; i++)  {
		    Mach_MonPutChar(syslogBuffer[i]);
		}
	    }
	    firstIndex = -1;
	    lastIndex = -1;
	}
    }
    MASTER_UNLOCK(syslogMutex);
}


/*
 *----------------------------------------------------------------------
 *
 * Dev_SyslogIOControl --
 *
 *	Perform device-specific functions with the console.
 *
 * Results:
 *	GEN_NOT_IMPLEMENTED if io control not supported.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/* ARGSUSED */
ReturnStatus
Dev_SyslogIOControl(devicePtr, command, inBufSize, inBuffer, outBufSize,
		     outBuffer)
    Fs_Device	        *devicePtr;
    int			command;
    int			inBufSize;
    Address		inBuffer;
    int			outBufSize;
    Address		outBuffer;
{
    ReturnStatus	status = SUCCESS;

    MASTER_LOCK(syslogMutex);
    switch (command) {
	case	IOC_REPOSITION:
	    /*
	     * Reposition doesn't matter
	     */
	    break;
	case	IOC_GET_FLAGS:
	case	IOC_SET_FLAGS:
	case	IOC_SET_BITS:
	case	IOC_CLEAR_BITS:
	    /*
	     * No syslog specific bits are set this way.
	     */
	    break;
	case	IOC_TRUNCATE:
	    firstIndex = -1;
	    lastIndex = - 1;
	    break;

	default:
	    status = GEN_NOT_IMPLEMENTED;
    }
    MASTER_UNLOCK(syslogMutex);
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Dev_SyslogSelect --
 *
 *	Perform device-specific select functions with the console.
 *
 * Results:
 *	SUCCESS	
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Dev_SyslogSelect(devicePtr, inFlags, outFlagsPtr)
    Fs_Device	        *devicePtr;
    int			inFlags;
    int			*outFlagsPtr;
{
    MASTER_LOCK(syslogMutex);
    if (inFlags & FS_READABLE) {
	if (lastIndex != -1) {
	    *outFlagsPtr |= FS_READABLE;
	}
    }
    if (inFlags & FS_WRITABLE) {
	if (firstIndex != lastIndex + 1) {
	    *outFlagsPtr |= FS_WRITABLE;
	}
    }
    MASTER_UNLOCK(syslogMutex);
    return(SUCCESS);
}

static	Boolean	savedOpenForReading;
static	int	syslogDebugCount = 0;


/*
 *----------------------------------------------------------------------
 *
 * Dev_SyslogDebug --
 *
 *	Start or stop redirecting output to the log buffer.  This is intended
 *	to be called by Dbg_Main when entering and leaving the debugger and
 *	Sys_Panic on a FATAL panic so that debugger output will go on the
 *	console since no process can run to flush the buffer.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Redirecting output to the log buffer is turned off or on.
 *
 *----------------------------------------------------------------------
 */
void
Dev_SyslogDebug(stopLog)
    Boolean	stopLog;	/* TRUE => stop storing log output into the
				 *         buffer. */
{
    if (stopLog) {
	if (syslogDebugCount == 0) {
	    savedOpenForReading = openForReading;
	    openForReading = FALSE;
	}
	syslogDebugCount++;
    } else {
	syslogDebugCount--;
	if (syslogDebugCount == 0) {
	    openForReading = savedOpenForReading;
	}
    }
}



/*
 *----------------------------------------------------------------------
 *
 * Dev_SyslogReturnBuffer --
 *
 *	Return the data in the syslog buffer to the caller.  Intended to be
 *	called by the debugger.
 *
 * Results:
 *	Pointer to buffer and the first and last indices.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
Dev_SyslogReturnBuffer(bufPtrPtr, firstIndexPtrPtr, lastIndexPtrPtr, bufSizePtr)
    char	**bufPtrPtr;
    int		**firstIndexPtrPtr;
    int		**lastIndexPtrPtr;
    int		*bufSizePtr;
{
    *bufPtrPtr = syslogBuffer;
    *firstIndexPtrPtr = &firstIndex;
    *lastIndexPtrPtr = &lastIndex;
    *bufSizePtr = SYSLOG_BUF_SIZE;
}

