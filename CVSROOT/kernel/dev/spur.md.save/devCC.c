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
#include "devCC.h"

/*
** Current Counter Mode
*/
int devCounterMode[MACH_MAX_NUM_PROCESSORS];
int devCounterLastMode[MACH_MAX_NUM_PROCESSORS];

/*
 * Forward routines. 
 */
static ReturnStatus RemoteCCWriteBytesFromRegs();
static ReturnStatus RemoteCCReadBytesFromRegs();
static ClientData RemoteSetModeReg();
ReturnStatus Dev_CCSetCounters();

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
 * Dev_CCReopen --
 *
 *	Reopen the /dev/cc device.  This uses Dev_CCReopen.
 *
 * Results:
 *	SUCCESS		- the device was opened.
 * 	DEV_OFFLINE	- the device unit number pointed to a
 *			  nonacitve processor.
 *
 * Side effects:
 *	None here, see Dev_CCOpen.
 *
 *----------------------------------------------------------------------
 */


/*ARGSUSED*/
ReturnStatus
Dev_CCReopen(devicePtr, refs, writes, notifyToken)
    Fs_Device *devicePtr;	/* Specifies type and unit number. */
    int refs;			/* Number of streams being reopened */
    int writes;			/* Number of write streams */
    ClientData notifyToken;	/* Used for Fs call-back to notify waiting
				 * processes that the console device is ready.*/
{
    int useFlags = FS_READ;
    if (writes) {
	useFlags |= FS_WRITE;
    }
    return Dev_CCOpen(devicePtr, useFlags, notifyToken) ;
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
Dev_CCRead(devicePtr, readPtr, replyPtr)
    Fs_Device	*devicePtr;
    Fs_IOParam	*readPtr;	/* Read parameter block */
    Fs_IOReply	*replyPtr;	/* Return length and signal */ 
{
    ReturnStatus 	status = SUCCESS;
    unsigned int	startOffset;
    int		 	bytesToRead;
    int			pnum;

    startOffset = (unsigned int) readPtr->offset;
    bytesToRead = readPtr->length;
    pnum = devicePtr->unit;
    if (startOffset > DEV_CC_MAX_OFFSET) {
	replyPtr->length = 0;
	return (SUCCESS);
    }
    if (startOffset + bytesToRead > DEV_CC_MAX_OFFSET) {
	bytesToRead = DEV_CC_MAX_OFFSET - startOffset;
    }
    if (pnum == Mach_GetProcessorNumber()) {
	    DevCCReadBytesFromRegs(startOffset, bytesToRead, readPtr->buffer);
    } else {
	    status = RemoteCCReadBytesFromRegs(pnum,startOffset, bytesToRead, 
					       readPtr->buffer);
    }
    replyPtr->length = bytesToRead;
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
Dev_CCWrite(devicePtr, writePtr, replyPtr)
    Fs_Device	*devicePtr;	/* Indicates device */	
    Fs_IOParam	*writePtr;	/* Standard write parameter block */
    Fs_IOReply	*replyPtr;	/* Return length and signal */
{
    ReturnStatus status;
    unsigned int	startOffset;
    int			pnum;
    int			bufSize;

    startOffset = (unsigned int) writePtr->offset;
    pnum = devicePtr->unit;
    bufSize = writePtr->length;
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
	    DevCCWriteBytesFromRegs(startOffset, bufSize, writePtr->buffer);
	    status = SUCCESS;
    } else {
	    status = RemoteCCWriteBytesFromRegs(pnum,startOffset, bufSize, 
				writePtr->buffer);

    }
    replyPtr->length = bufSize;
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
Dev_CCIOControl(devicePtr, ioctlPtr, replyPtr)
    Fs_Device	        *devicePtr;
    Fs_IOCParam		*ioctlPtr;
    Fs_IOReply		*replyPtr;
{
    ReturnStatus	status;
    int			pnum = devicePtr->unit;

    switch (ioctlPtr->command) {
	case 	IOC_CCDEV_SET_MODE: {
	    unsigned int	newMode, oldMode; 
	    if (ioctlPtr->inBufSize != sizeof(char)) {
		return (GEN_INVALID_ARG);
	    } 
	    newMode = *(unsigned char *) ioctlPtr->inBuffer;
	    devCounterMode[pnum] = newMode;
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

	    if (ioctlPtr->outBufSize != sizeof(Timer_Ticks)) {
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
	    bcopy((char *) &counter, ioctlPtr->outBuffer, sizeof(Timer_Ticks));
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
	    int		bytesWritten;
	    ReturnStatus	status;
	    Fs_IOParam	write;
	    Fs_IOReply   reply;

	    if (ioctlPtr->inBufSize != sizeof(int)) {
		return (GEN_INVALID_ARG);
	    }
	    bcopy(ioctlPtr->inBuffer,(char *) &write.offset,sizeof(int));
	    write.length = DEV_CC_MAX_OFFSET;
	    write.buffer = Zero;
	    status = Dev_CCWrite(devicePtr, &write, &reply);
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

/*
 * The following code and variables are used by the processed cache controller
 * device. The pcc maintains 64bit version of 32 bit counters in the normal
 * cc device.
 */

#include "timer.h"

Sync_Semaphore	devPCCMutex = Sync_SemInitStatic("devPCCMutex");

/*
 * Timer queue element for updating PCC 64bit counters.
 */

static Timer_QueueElement  UpdateTimerElement;
static Boolean UpdateTimerStarted = FALSE;

/*
 * Memory image of the pcc device.
 */
static PCCdev PCCdevMem;

/*
 *----------------------------------------------------------------------
 *
 *  DoPCCMemUpdate --
 *
 *      Update the memory image of the 64bit counters of the processed
 *	cache controller structure.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

static void
DoPCCMemUpdate()
{
     register 		CCdev64bitCounter	*counter;
     register 		int			c;
     int		pnum;
     Fs_Device	        device;
     Fs_Device	        *devicePtr = &device;
     CCdev		newCCdev;
     int		len;
     ReturnStatus	status;
     register int       modeReg;		/* Mode register to restore. */
     Fs_IOParam	read;
     Fs_IOReply	reply;	
     Fs_IOCParam ioctl;
     int cpu = Mach_GetProcessorNumber();

     for (pnum = 0; pnum < mach_NumProcessors; pnum++) {
        devicePtr->unit = pnum;
	/*
	 * For each processor we read the t0 register.
	 */
	ioctl.command = IOC_CCDEV_READ_T0;
	ioctl.inBufSize = 0;
	ioctl.inBuffer = NULL;
	ioctl.outBufSize = sizeof(CCdev64bitCounter);
	ioctl.outBuffer = (Address) &(PCCdevMem[pnum].t0);
	status =  Dev_CCIOControl(devicePtr, &ioctl, &reply);
	/*
	 * And the cache controller registers.
	 *  Turn them off first so we don't get a corrupt sample.
	 */
	read.length = sizeof(newCCdev);
	read.buffer = (Address) &newCCdev;
	read.offset = 0;
	if (pnum == cpu) {
	    Dev_CCSetCounters(COUNTERS_OFF);
	    status =  Dev_CCRead(devicePtr,&read, &reply);
	    Dev_CCSetCounters(COUNTERS_RESTORE_LAST);
	} else {
	    status = Mach_CallProcessor(pnum, Dev_CCSetCounters, 
			(ClientData) COUNTERS_OFF, TRUE, (ClientData) NIL);
	    if (status != SUCCESS) {
		printf("CPC call to idle counters failed: 0x%x\n", status);
	    }
	    status =  Dev_CCRead(devicePtr,&read, &reply);
	    if (status != SUCCESS) {
		printf("Read of CC failed: 0x%x\n", status);
		return;
	    }
	    status = Mach_CallProcessor(pnum, Dev_CCSetCounters, 
			(ClientData) COUNTERS_RESTORE_LAST, TRUE, 
			(ClientData) NIL);
	    if (status != SUCCESS) {
		printf("CPC call to activate counters failed: 0x%x\n", status);
	    }
	}
	/*
	 * Update any counter that wraps.
	 */
	counter = PCCdevMem[pnum].c;
	for (c = 0; c < 16; c++) {
	    if (newCCdev.c[c].value <  counter->low) {
		counter->high++;
	    }
	    counter->low = newCCdev.c[c].value;
	    counter++;
	}

     }

}

/*
 *----------------------------------------------------------------------
 *
 * Update64BitCounters --
 *
 *      Update the 64bit counters of the process cache controller structure.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

static void
Update64BitCounters()
{
    MASTER_LOCK(&devPCCMutex);
    DoPCCMemUpdate();
    MASTER_UNLOCK(&devPCCMutex);
    Timer_ScheduleRoutine(&UpdateTimerElement, TRUE);
}


/*
 *----------------------------------------------------------------------
 *
 * Dev_PCCOpen --
 *
 *	Open the /dev/pcc device.  
 *
 * Results:
 *	SUCCESS		- the device was opened.
 *
 * Side effects:
 *	The cc is "opened" and static variables are initialized.
 *
 *----------------------------------------------------------------------
 */


/*ARGSUSED*/
ReturnStatus
Dev_PCCOpen(devicePtr, useFlags, notifyToken)
    Fs_Device *devicePtr;	/* Specifies type and unit number. */
    int useFlags;		/* Flags from the stream being opened */
    ClientData notifyToken;	/* Used for Fs call-back to notify waiting
				 * processes that the console device is ready.*/
{
    /*
     * If not already started, start call back to keep 64bit counters upto 
     * date. Since the minumum wrap time is 7 mins, we sample every 5 mins
     * to insure we don't miss any wraps.
     */
    MASTER_LOCK(&devPCCMutex);
    Sync_SemRegister(&devPCCMutex);
    if (! UpdateTimerStarted) {
	UpdateTimerElement.routine = Update64BitCounters;
	UpdateTimerElement.clientData = 0;
	UpdateTimerElement.interval = timer_IntOneMinute*5;
	UpdateTimerStarted = TRUE;
	Timer_ScheduleRoutine(&UpdateTimerElement, TRUE);
    }
    MASTER_UNLOCK(&devPCCMutex);

    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_PCCReopen --
 *
 *	Reopen the /dev/pcc device.  This uses Dev_PCCReopen.
 *
 * Results:
 *	SUCCESS		- the device was opened.
 * 	DEV_OFFLINE	- the device unit number pointed to a
 *			  nonacitve processor.
 *
 * Side effects:
 *	None here, see Dev_PCCOpen.
 *
 *----------------------------------------------------------------------
 */


/*ARGSUSED*/
ReturnStatus
Dev_PCCReopen(devicePtr, refs, writes, notifyToken)
    Fs_Device *devicePtr;	/* Specifies type and unit number. */
    int refs;			/* Number of streams being reopened */
    int writes;			/* Number of write streams */
    ClientData notifyToken;	/* Used for Fs call-back to notify waiting
				 * processes that the console device is ready.*/
{
    int useFlags = FS_READ;
    if (writes) {
	useFlags |= FS_WRITE;
    }
    return Dev_PCCOpen(devicePtr, useFlags, notifyToken) ;
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_PCCRead --
 *
 *	Read data from the PCC registers.  
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
Dev_PCCRead(devicePtr, readPtr, replyPtr)
    Fs_Device	*devicePtr;	/* Indicates device */
    Fs_IOParam	*readPtr;	/* Read parameter block */
    Fs_IOReply	*replyPtr;	/* Return length and signal */ 
{
    ReturnStatus 	status = SUCCESS;
    unsigned int	startOffset;
    int		 	bytesToRead;

    startOffset = (unsigned int) readPtr->offset;
    bytesToRead = readPtr->length;
    if (startOffset > sizeof(PCCdev)) {
	replyPtr->length = 0;
	return (SUCCESS);
    }
    if (startOffset + bytesToRead > sizeof(PCCdev)) {
	bytesToRead = sizeof(PCCdev) - startOffset;
    }
    /*
     * Update the memory image and copy it out.
     */
    MASTER_LOCK(&devPCCMutex);
    DoPCCMemUpdate();
    bcopy(((char *) &PCCdevMem)+startOffset,readPtr->buffer,bytesToRead);
    MASTER_UNLOCK(&devPCCMutex);

    replyPtr->length = bytesToRead;
    return (SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Dev_PCCWrite --
 *
 *	Write characters to the register of the  SPUR PCC. 
 *
 * Results:
 *	 DEV_NO_DEVICE - This is not implemented yet.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
ReturnStatus
Dev_PCCWrite(devicePtr, writePtr, replyPtr)
    Fs_Device	*devicePtr;	/* Indicates device */	
    Fs_IOParam	*writePtr;	/* Standard write parameter block */
    Fs_IOReply	*replyPtr;	/* Return length and signal */
{

    return (DEV_NO_DEVICE);
}



/*
 *----------------------------------------------------------------------
 *
 * Dev_PCCClose --
 *
 *	Close the SPUR PCC.
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
Dev_PCCClose(devicePtr, useFlags, openCount, writerCount)
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
 * Dev_PCCIOControl --
 *
 *	Perform device-specific functions with the PCC.
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
Dev_PCCIOControl(devicePtr, ioctlPtr, replyPtr)
    Fs_Device	        *devicePtr;
    Fs_IOCParam		*ioctlPtr;
    Fs_IOReply		*replyPtr;
{
    ReturnStatus	status;
    int			pnum;

    switch (ioctlPtr->command) {
	case 	IOC_CCDEV_SET_MODE: {
    	    Fs_Device	        device;
    	    Fs_Device	        *newDevicePtr = &device;
	    /*
	     * Set the mode of all processors.
	     */
	    device = *devicePtr;
	    for (pnum = 0; pnum < mach_NumProcessors; pnum++) {
		newDevicePtr->unit = pnum;
		status = Dev_CCIOControl(newDevicePtr, ioctlPtr, replyPtr);
	     }
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
    	    Fs_Device	        device;
    	    Fs_Device	        *newDevicePtr = &device;
	    Fs_IOCParam		ioctl;
	    int		pnum;
		/*
		 * Zero  counters. 
		 */
	    device = *devicePtr;
	    ioctl = *ioctlPtr;
	    ioctl.command = IOC_TRUNCATE;
	    MASTER_LOCK(&devPCCMutex);
	    for(pnum = 0; pnum < mach_NumProcessors; pnum++) { 
		newDevicePtr->unit = pnum;
		status = Dev_CCIOControl(newDevicePtr, &ioctl, replyPtr);
	    }
	    bzero((char *) &PCCdevMem,sizeof(PCCdevMem));
	    MASTER_UNLOCK(&devPCCMutex);
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
 * Dev_PCCSelect --
 *
 *	Perform device-specific select functions with the PCC.
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
Dev_PCCSelect(devicePtr, inFlags, outFlagsPtr)
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
 * Dev_CCSetCounters --
 *
 *	Set the performance counters of the current CPU. This routine is
 * 	called from the IdleLoop in sched.c to remove the IdleLoop from
 *	the counters.
 *
 * Results:
 *	The old mode register
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
ReturnStatus
Dev_CCSetCounters(action)
	register int	action;	/* TRUE -> restore old mode */
{
    register int	oldMode,newMode;
    int		cpu = Mach_GetProcessorNumber();

    Mach_DisableNonmaskableIntr();
    oldMode = Mach_Read8bitCCReg(MACH_MODE_REG);
    switch(action) {
	case COUNTERS_OFF: 
	    newMode = ~MACH_MODE_PERF_COUNTER_MASK & oldMode;
	    break;
	case COUNTERS_RESTORE:
	    newMode = devCounterMode[cpu];
	    break;
	case COUNTERS_RESTORE_LAST:
	    newMode = devCounterLastMode[cpu];
	    break;
	default:
	    panic("Illegal action to Dev_CCSetCounters\n");
    }
    devCounterLastMode[cpu] = oldMode;
    Mach_Write8bitCCReg(MACH_MODE_REG,newMode);
    Mach_EnableNonmaskableIntr();
    return SUCCESS;
}

