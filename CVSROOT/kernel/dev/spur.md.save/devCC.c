 /* 
 * devCC.c --
 *
 *	Device driver for SPUR cache controller.  Provides 
 *	open/close/read/write/ioctl interface for reading the controller's
 *	register and turning on and off counters.
 *
 *			
 * Copyright 1986, 1988 Regents of the University of California
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
#include "dev.h"
#include "dev/ccdev.h"
#include "fs.h"
#include "sys.h"
#include "mach.h"
#include "machCCRegs.h"
#include "timer.h"
#include "varargs.h"


/*
 * Forward routines. 
 */
static ReturnStatus RemoteCCWriteBytesFromRegs();
static ReturnStatus RemoteCCReadBytesFromRegs();
static ClientData RemoteSetModeReg();

/*
 *----------------------------------------------------------------------
 *
 * Dev_CCOpen --
 *
 *	Open the /dev/cc device.  
 *
 * Results:
 *	SUCCESS		- the device was opened.
 * 	DEV_OFFLINE	- the device unit number pointed to a
 *			  nonacitve processor.
 *
 * Side effects:
 *	The cc is "opened" and static variables are initialized.
 *
 *----------------------------------------------------------------------
 */


/*ARGSUSED*/
ReturnStatus
Dev_CCOpen(devicePtr, useFlags, notifyToken)
    Fs_Device *devicePtr;	/* Specifies type and unit number. */
    int useFlags;		/* Flags from the stream being opened */
    ClientData notifyToken;	/* Used for Fs call-back to notify waiting
				 * processes that the console device is ready.*/
{
    ReturnStatus 	status;
    unsigned int	pnum;

    pnum = devicePtr->unit;
    if (pnum > MACH_MAX_NUM_PROCESSORS ||
        mach_ProcessorStatus[pnum] != MACH_ACTIVE_STATUS) {
	status = DEV_OFFLINE;
    } else {
        status = SUCCESS;
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_CCRead --
 *
 *	Read data from the CC registers.  
 *
 * Results:
 *	SUCCESS		- the data was obtained.
 *	FAILURE		- an invalid argument.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Dev_CCRead(devicePtr, offset, bufSize, bufPtr, lenPtr)
    Fs_Device	*devicePtr;
    int		offset;	  	/* Offset to start read */
    int		bufSize;	/* Size of buffer. */
    Address	bufPtr;		/* Place to store data. */
    register int *lenPtr;  	/* Maximum number of chars to read 
				 * before returning. */ 
{
    ReturnStatus 	status = SUCCESS;
    unsigned int	startOffset = (unsigned int) offset;
    int		 	bytesToRead = *lenPtr;
    int			pnum = devicePtr->unit;

    if (startOffset > DEV_CC_MAX_OFFSET) {
	*lenPtr = 0;
	return (SUCCESS);
    }
    if (startOffset + bytesToRead > DEV_CC_MAX_OFFSET) {
	bytesToRead = DEV_CC_MAX_OFFSET - startOffset;
    }
    if (pnum == Mach_GetProcessorNumber()) {
	    DevCCReadBytesFromRegs(startOffset, bytesToRead, bufPtr);
    } else {
	    status = RemoteCCReadBytesFromRegs(pnum,startOffset, bytesToRead, 
					       bufPtr);
    }
    *lenPtr = bytesToRead;
    return (SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Dev_CCWrite --
 *
 *	Write characters to the register of the  SPUR CC. 
 *
 * Results:
 *	SUCCESS		- 
 *	FS_INVALID_ARG  - 
 *
 * Side effects:
 *	Data is written to the console or serial lines.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
ReturnStatus
Dev_CCWrite(devicePtr, offset, bufSize, bufPtr, bytesWrittenPtr)
    Fs_Device     *devicePtr;	
    int		 offset;		/* Ignored. */
    int		 bufSize;		/* Number of bytes to write. */
    register Address  bufPtr;		/* Place to find data */
    int 	 *bytesWrittenPtr;	/* Number of chars written */ 
{
    ReturnStatus status;
    unsigned int	startOffset = (unsigned int) offset;
    int			pnum = devicePtr->unit;

    if (startOffset > DEV_CC_MAX_OFFSET ) {
	return (SUCCESS);
    }
    if (startOffset < DEV_CC_MIN_USER_OFFSET) {
	bufSize -= (DEV_CC_MIN_USER_OFFSET - startOffset);
	if (bufSize < 0) {
		return(SUCCESS);
	}
	startOffset = DEV_CC_MIN_USER_OFFSET;
    }
    if (startOffset + bufSize > DEV_CC_MAX_OFFSET) {
	bufSize = DEV_CC_MAX_OFFSET - startOffset;
    }

    if (pnum == Mach_GetProcessorNumber()) {
	    DevCCWriteBytesFromRegs(startOffset, bufSize, bufPtr);
	    status = SUCCESS;
    } else {
	    status = RemoteCCWriteBytesFromRegs(pnum,startOffset, bufSize, 
				bufPtr);

    }
    *bytesWrittenPtr = bufSize;
    return (status);
}



/*
 *----------------------------------------------------------------------
 *
 * Dev_CCClose --
 *
 *	Close the SPUR CC.
 *
 * Results:
 *	SUCCESS		- always returned.
 *
 * Side effects:
 *	The CC is "closed".
 *
 *----------------------------------------------------------------------
 */
/* ARGSUSED */
ENTRY ReturnStatus
Dev_CCClose(devicePtr, useFlags, openCount, writerCount)
    Fs_Device	*devicePtr;	/* Device information. */
    int		useFlags;	/* FS_READ | FS_WRITE */
    int		openCount;	/* Number of times still open. */
    int		writerCount;	/* Number of times still open for writing. */
{

    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Dev_CCIOControl --
 *
 *	Perform device-specific functions with the CC.
 *
 * Results:
 *	SUCCESS		- for DEV_EVENT_UNIT, always returned.
 *	GEN_INVALID_ARG	- bad buffer size.
 *
 * Side effects:
 *	Variable.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Dev_CCIOControl(devicePtr, command, inBufSize, inBuffer, outBufSize,
		     outBuffer)
    Fs_Device	        *devicePtr;
    int			command;
    int			inBufSize;
    Address		inBuffer;
    int			outBufSize;
    Address		outBuffer;
{
    ReturnStatus	status;
    int			pnum = devicePtr->unit;

    switch (command) {
	case 	IOC_CCDEV_SET_MODE: {
	    unsigned int	newMode, oldMode; 
	    if (inBufSize != sizeof(char)) {
		return (GEN_INVALID_ARG);
	    } 
	    newMode = *(unsigned char *) inBuffer;
	    if (pnum == Mach_GetProcessorNumber()) { 
		    Mach_DisableNonmaskableIntr();
		    oldMode = Mach_Read8bitCCReg(MACH_MODE_REG);
		    newMode = (newMode & DEV_CC_USER_MODE_BITS) |
					(oldMode & ~DEV_CC_USER_MODE_BITS);
		    Mach_Write8bitCCReg(MACH_MODE_REG,newMode);
		    Mach_EnableNonmaskableIntr();
		    status = SUCCESS;
	    } else {
		status = Mach_CallProcessor(pnum, RemoteSetModeReg,
				(ClientData)newMode, TRUE, (ClientData *) NIL);
	    }
	    return (status);
	}

	case 	IOC_CCDEV_READ_T0: {
	    unsigned int	newMode, oldMode; 
	    Timer_Ticks		counter;
	    extern void Timer_ReadT0();

	    if (outBufSize != sizeof(Timer_Ticks)) {
		return (GEN_INVALID_ARG);
	    } 
	    if (pnum == Mach_GetProcessorNumber()) {
		Timer_ReadT0(&counter);
		status = SUCCESS;
	    } else {
	       status = Mach_CallProcessor(pnum, (ClientData (*)())Timer_ReadT0,
					 (ClientData) &counter,
					 TRUE, (ClientData *) NIL);
	    }
	    bcopy((char *) &counter, outBuffer, sizeof(Timer_Ticks));
	    return (status);
	}

	case	IOC_REPOSITION:
	    /*
	     * Reposition doesn't matter
	     */
	    return (SUCCESS);

	    /*
	     * No CC specific bits are set this way.
	     */
	case	IOC_GET_FLAGS:
	case	IOC_SET_FLAGS:
	case	IOC_SET_BITS:
	case	IOC_CLEAR_BITS:
	    break;

	case	IOC_SET_OWNER:
	case	IOC_GET_OWNER: 
	     return(GEN_NOT_IMPLEMENTED);

	case	IOC_TRUNCATE: {
		/*
		 * Zero  counters. 
		 */
	    static char Zero[DEV_CC_MAX_OFFSET];
	    int		startOffset, bytesWritten;
	    ReturnStatus	status;

	    if (inBufSize != sizeof(int)) {
		return (GEN_INVALID_ARG);
	    }
	    bcopy(inBuffer,(char *) &startOffset,sizeof(int));
	    status = Dev_CCWrite(devicePtr, startOffset, DEV_CC_MAX_OFFSET, 
					Zero, &bytesWritten);
	    return(status);
	}
	case	IOC_LOCK:
	case	IOC_UNLOCK:
	    return(GEN_NOT_IMPLEMENTED);

	case	IOC_NUM_READABLE:
	    return(GEN_NOT_IMPLEMENTED);

	case	IOC_MAP:
	    return(GEN_NOT_IMPLEMENTED);
	default:
	    return(GEN_INVALID_ARG);
    }
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Dev_CCSelect --
 *
 *	Perform device-specific select functions with the CC.
 *
 * Results:
 *	SUCCESS		- always returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
ReturnStatus
Dev_CCSelect(devicePtr, inFlags, outFlagsPtr)
    Fs_Device	        *devicePtr;
    int			inFlags;
    int			*outFlagsPtr;
{
    *outFlagsPtr = inFlags;
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 *  RemoteReadCache --
 *
 *	Read bytes from the current processor's cache controller registers.
 *	this routine is called via the CPC machanism.
 *
 * Results:
 *	SUCCESS		- the read was worked.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
static ClientData
RemoteReadCache(vargsPtr)
    va_list	*vargsPtr;
{
     unsigned int startOffset;
     int	  bytesToRead;
     Address	  bufPtr;

    startOffset = va_arg(*vargsPtr,unsigned int);
    bytesToRead = va_arg(*vargsPtr,int);
    bufPtr = va_arg(*vargsPtr,Address);

    DevCCReadBytesFromRegs(startOffset, bytesToRead, bufPtr);
    return (ClientData) SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 *  RemoteWriteCache --
 *
 *	Write bytes from to current processor's cache controller registers.
 *	This routine is called via the CPC machanism.
 *
 * Results:
 *	SUCCESS		- the write was worked.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
static ClientData
RemoteWriteCache(vargsPtr)
    va_list	*vargsPtr;
{
     unsigned int startOffset;
     int	  bufSize;
     Address	  bufPtr;

    startOffset = va_arg(*vargsPtr,unsigned int);
    bufSize = va_arg(*vargsPtr,int);
    bufPtr = va_arg(*vargsPtr,Address);

    DevCCWriteBytesFromRegs(startOffset, bufSize, bufPtr);
    return (ClientData) SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * RemoteCCReadBytesFromRegs --
 *
 *	Reads bytes from a cache controller on a different processor.
 *
 * Results:
 *	SUCCESS		- the read was worked.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
RemoteCCReadBytesFromRegs(va_alist /* pnum,startOffset, bytesToRead, bufPtr*/)
    va_dcl
{
    int 	pnum; 
    va_list	vargs;
    ReturnStatus status;

    va_start(vargs);
    pnum = va_arg(vargs,int);
    status = Mach_CallProcessor(pnum, RemoteReadCache, (ClientData) &vargs,
				TRUE, (ClientData *) NIL);
    return (status);
}


/*
 *----------------------------------------------------------------------
 *
 * RemoteCCWriteBytesFromRegs --
 *
 *	Write bytes to a cache controller on a different processor.
 *
 * Results:
 *	SUCCESS		- the read was worked.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
RemoteCCWriteBytesFromRegs(va_alist /* pnum,startOffset, bugSize, bufPtr*/)
    va_dcl
{
    int 	pnum; 
    va_list	vargs;
    ReturnStatus status;

    va_start(vargs);
    pnum = va_arg(vargs,int);
    status = Mach_CallProcessor(pnum, RemoteWriteCache, (ClientData) &vargs,
				TRUE, (ClientData *) NIL);
    return (status);
}



/*
 *----------------------------------------------------------------------
 *
 * RemoteSetModeReg --
 *
 *	Set the mode register on  the current processor. Called as a CPC.
 *
 * Results:
 *	SUCCESS		- the operation was worked.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
static ClientData
RemoteSetModeReg(newMode)
     unsigned int	newMode;
{
    unsigned int oldMode;	

    Mach_DisableNonmaskableIntr();
    oldMode = Mach_Read8bitCCReg(MACH_MODE_REG);
    newMode = (newMode & DEV_CC_USER_MODE_BITS) |
			(oldMode & ~DEV_CC_USER_MODE_BITS);
    Mach_Write8bitCCReg(MACH_MODE_REG,newMode);
    Mach_EnableNonmaskableIntr();
    return (ClientData) SUCCESS;
}
	
