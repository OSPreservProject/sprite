/* 
 * devXbus.c --
 *
 *	Routines used for running the xbus board that the RAID project
 *	built.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /cdrom/src/kernel/Cvsroot/kernel/dev/sun4.md/devXbus.c,v 9.3 92/10/23 15:04:43 elm Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "sprite.h"
#include "stdio.h"
#include "mach.h"
#include "dev.h"
#include "devInt.h"
#include "stdlib.h"
#include "vmMach.h"
#include "dev/xbus.h"
#include "devXbusInt.h"
#include "devXbus.h"
#include "dbg.h"
#include "sync.h"

int		devXbusDebug = FALSE;
static int		devXbusModuleInitted = FALSE;
static DevXbusInfo	*xbusInfo[DEV_XBUS_MAX_BOARDS];
static Sync_Condition	xorTestDone;


/*----------------------------------------------------------------------
 *
 * DevXbusResetBoard
 *
 *	Reset the xbus board by poking the reset registers.  Get the
 *	mutex semaphore first, though.
 *
 *----------------------------------------------------------------------
 */
static
ReturnStatus
DevXbusResetBoard (infoPtr)
DevXbusInfo*	infoPtr;
{
    ReturnStatus status = SUCCESS;
    DevXbusCtrlRegs *regPtr = infoPtr->regs;
    unsigned int resetValue;

    if (devXbusDebug) {
	printf ("DevXbusResetBoard: resetting %s.\n", infoPtr->name);
    }
    resetValue = DEV_XBUS_RESETREG_RESET;
    status = Mach_Probe (sizeof (regPtr->reset),(Address)&resetValue,
			 (Address)&(regPtr->reset));
			 
    if (status != SUCCESS) {
	goto resetDone;
    }
    MACH_DELAY (500000);
    resetValue = infoPtr->resetValue;
    status = Mach_Probe (sizeof (regPtr->reset), (Address)&resetValue,
			 (Address)&(regPtr->reset));

    MASTER_LOCK (&infoPtr->mutex);
    infoPtr->qHead = infoPtr->qTail = infoPtr->xorQueue;
    infoPtr->qEnd = &(infoPtr->xorQueue[DEV_XBUS_MAX_XOR_BUFS]);
    infoPtr->numInQ = 0;
    infoPtr->state = DEV_XBUS_STATE_OK;
  resetDone:
    MASTER_UNLOCK (&infoPtr->mutex);
    if (devXbusDebug) {
	printf ("DevXbusResetBoard: done, status = 0x%x\n", status);
    }
    return (status);
}


/*
 *----------------------------------------------------------------------
 *
 * DevXbusInit
 *
 *	Initialize the xbus board and set up necessary data structures.
 *
 *----------------------------------------------------------------------
 */
ENTRY ClientData
DevXbusInit (ctrlPtr)
DevConfigController *ctrlPtr;
{
    ReturnStatus	status = SUCCESS;
    DevXbusInfo*	infoPtr = NULL;
    DevXbusCtrlRegs*	regPtr = (DevXbusCtrlRegs *)ctrlPtr->address;
    int			boardNum;
    unsigned int	resetVal;
    int			i;

    boardNum = ctrlPtr->controllerID;
    if (!devXbusModuleInitted) {
	int i;
	for (i = 0; i < DEV_XBUS_MAX_BOARDS; i++) {
	    xbusInfo[i] = NULL;
	}
	devXbusModuleInitted = TRUE;
    }

    if (boardNum >= DEV_XBUS_MAX_BOARDS) {
	status = DEV_NO_DEVICE;
	goto initExit;
    }

    /*
     * Try to find the board by probing the reset register.
     */
    resetVal = DEV_XBUS_RESETREG_RESET;
    if ((status = Mach_Probe (sizeof (regPtr->reset), (char*)&resetVal,
			      (char*)&(regPtr->reset))) != SUCCESS) {
	if (devXbusDebug) {
	    printf ("DevXbusInit: %s not responding to probe.\n",
		    ctrlPtr->name);
	}
	goto initExit;
    }

    if ((infoPtr = (DevXbusInfo *) malloc (sizeof (DevXbusInfo))) == NULL) {
	printf ("DevXbusInit: Couldn't allocate space for board info.\n");
	status = FAILURE;
	goto initExit;
    }

    xbusInfo[boardNum] = infoPtr;
    infoPtr->regs = regPtr;
    infoPtr->state = 0;
    infoPtr->name = ctrlPtr->name;
    infoPtr->addressBase = Dev_XbusAddressBase (boardNum);
    infoPtr->boardId = boardNum;
    infoPtr->resetValue = DEV_XBUS_RESETREG_NORMAL;
    for (i = 0; i < DEV_XBUS_MALLOC_NUM_SIZES; i++) {
	infoPtr->freeList[i] = NULL;
    }
    infoPtr->freePtrList = NULL;

    if ((infoPtr->hippisCtrlFifo = (vuint *)VmMach_MapInDevice
	 ((Address)(infoPtr->addressBase + DEV_XBUS_REG_HIPPIS_CTRL_FIFO),
	  DEV_XBUS_ADDR_SPACE)) == NULL) {
	if (devXbusDebug) {
	    printf ("%s couldn't map in hippis fifo.\n", ctrlPtr->name);
	}
	status = FAILURE;
	goto initExit;
    }
    if ((infoPtr->hippidCtrlFifo = (vuint *)VmMach_MapInDevice
	 ((Address)(infoPtr->addressBase + DEV_XBUS_REG_HIPPID_CTRL_FIFO),
	  DEV_XBUS_ADDR_SPACE)) == NULL) {
	if (devXbusDebug) {
	    printf ("%s couldn't map in hippid fifo.\n", ctrlPtr->name);
	}
	status = FAILURE;
	goto initExit;
    }
    if ((infoPtr->xorCtrlFifo = (vuint *)VmMach_MapInDevice
	 ((Address)(infoPtr->addressBase + DEV_XBUS_REG_XOR_CTRL_FIFO),
	  DEV_XBUS_ADDR_SPACE)) == NULL) {
	if (devXbusDebug) {
	    printf ("%s couldn't map in xor fifo.\n", ctrlPtr->name);
	}
	status = FAILURE;
	goto initExit;
    }

    sprintf (infoPtr->semName, "XbusMutex 0x%x", boardNum);
    Sync_SemInitDynamic (&infoPtr->mutex, infoPtr->semName);
    status = DevXbusResetBoard (infoPtr);
    if (status == SUCCESS) {
	infoPtr->state |= DEV_XBUS_STATE_OK;
    } else {
	status = DEV_NO_DEVICE;
    }

  initExit:
    if (status != SUCCESS) {
	printf ("DevXbusInit: Didn't find %s at at 0x%x\n", ctrlPtr->name,
		Dev_XbusAddressBase(boardNum));
    } else {
	printf ("Found %s (xbus%d) at 0x%x\n", ctrlPtr->name,
		boardNum, infoPtr->addressBase);
    }
    return ((status == SUCCESS) ? (ClientData)infoPtr : DEV_NO_CONTROLLER);
}

/*----------------------------------------------------------------------
 *
 * DevXbusStuffXor --
 *
 *	This procedure tries to stuff an XOR command into the xbus
 *	board.  It will only do so if there isn't already an XOR
 *	in progress.  The master lock for this xbus *must* be held
 *	when this is called.
 *
 *----------------------------------------------------------------------
 */
static
void
DevXbusStuffXor (infoPtr)
register DevXbusInfo*	infoPtr;
{
    register int		i;
    register DevXbusXorInfo*	xorPtr;
    register unsigned int*	curAddrPtr;

    if (devXbusDebug) {
	printf ("DevXbusStuffXor: stuffing %s, %d in queue.\n", infoPtr->name,
		infoPtr->numInQ);
    }
    if (!(infoPtr->state & DEV_XBUS_STATE_XOR_GOING) &&
	(infoPtr->numInQ != 0)) {
	infoPtr->state |= DEV_XBUS_STATE_XOR_GOING;
	xorPtr = infoPtr->qHead;
	/*
	 * XOR buffer size and buffer address are passed as long words.
	 * This means they must all be shifted right two bits.
	 */
	*(infoPtr->xorCtrlFifo) = xorPtr->bufLen >> 2;
	for (i = 0, curAddrPtr = xorPtr->buf; i < xorPtr->numBufs; i++) {
	    *(infoPtr->xorCtrlFifo) = *(curAddrPtr++) >> 2;
	}
	*(infoPtr->xorCtrlFifo) = (xorPtr->destBuf >> 2) | DEV_XBUS_XOR_GO;
    }
    if (devXbusDebug) {
	printf ("DevXbusStuffXor: exiting...\n");
    }
}

/*----------------------------------------------------------------------
 *
 * accessXbusRegister
 *
 *	Read or write an xbus board register.  By design, the register
 *	symbol is the offset from the start of the register area.
 *	Treat the FIFO registers separately, as they might not be
 *	contiguously mapped with the rest of the registers.
 *
 *----------------------------------------------------------------------
 */
static
ReturnStatus
accessXbusRegister (infoPtr, regNum, valuePtr, accessType)
DevXbusInfo	*infoPtr;
int		regNum;
unsigned int*	valuePtr;
int		accessType;
{
    ReturnStatus	status = SUCCESS;
    Address		regAddr;

    switch (regNum) {
      case DEV_XBUS_REG_HIPPID_CTRL_FIFO:
	regAddr = (Address)infoPtr->hippidCtrlFifo;
	break;
      case DEV_XBUS_REG_HIPPIS_CTRL_FIFO:
	regAddr = (Address)infoPtr->hippisCtrlFifo;
	break;
      case DEV_XBUS_REG_XOR_CTRL_FIFO:
	regAddr = (Address)infoPtr->xorCtrlFifo;
	break;
      default:
	if ((regNum >= 0) && (regNum <= DEV_XBUS_REG_STATUS)) {
	    regAddr = (Address)infoPtr->regs + regNum;
	} else {
	    status = GEN_INVALID_ARG;
	    goto endAccess;
	}
    }

    if (accessType == IOC_XBUS_READ_REG) {
	status = Mach_Probe (sizeof (*valuePtr), regAddr, (Address)valuePtr);
    } else if (accessType == IOC_XBUS_WRITE_REG) {
	status = Mach_Probe (sizeof (*valuePtr), (Address)valuePtr, regAddr);
    } else {
	status = GEN_INVALID_ARG;
    }
  endAccess:
    return (status);
}

/*----------------------------------------------------------------------
 *
 * DevXbusXor --
 *
 *	This is the internally callable xor queueing routine.  It assumes
 *	that the master lock is held before the routine is called.
 *
 * Returns:
 *	Standard Sprite status.
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
static
ReturnStatus
DevXbusXor (infoPtr, destBuf, numBufs, bufArrayPtr, bufLen, callbackProc,
	    clientData)
DevXbusInfo*	infoPtr;
unsigned int	destBuf;
unsigned int	numBufs;	
unsigned int* 	bufArrayPtr;
unsigned int	bufLen;
void		(*callbackProc)();
ClientData	clientData;
{
    ReturnStatus	status = SUCCESS;
    DevXbusXorInfo*	xorPtr;
    unsigned int*	bufCopyPtr;
    unsigned int	i;

    if (infoPtr->numInQ == DEV_XBUS_MAX_QUEUED_XORS) {
	status = FAILURE;
	if (devXbusDebug) {
	    printf ("Dev_XbusXor: XOR queue for %s full.\n", infoPtr->name);
	}
    } else if (numBufs > DEV_XBUS_MAX_XOR_BUFS) {
	status = GEN_INVALID_ARG;
    } else {
	xorPtr = infoPtr->qTail;
	infoPtr->qTail += 1;
	if (infoPtr->qTail == infoPtr->qEnd) {
	    infoPtr->qTail = infoPtr->xorQueue;
	}
	infoPtr->numInQ += 1;
	if (devXbusDebug) {
	    printf ("Dev_XbusXor: %s added XOR in position %d.\n",
		    infoPtr->name, infoPtr->qTail - infoPtr->xorQueue);
	    printf ("num bufs = 0x%x, dest buf: 0x%x\n", numBufs,
		    destBuf);
	}
	xorPtr->destBuf = destBuf;
	for (i = 0, bufCopyPtr = xorPtr->buf; i < numBufs; i++) {
	    if (devXbusDebug) {
		printf ("Src buffer at 0x%x\n", *bufArrayPtr);
	    }
	    *(bufCopyPtr++) = *(bufArrayPtr++);
	}
	xorPtr->callbackProc = callbackProc;
	xorPtr->clientData = clientData;
	xorPtr->bufLen = bufLen;
	xorPtr->numBufs = numBufs;
	xorPtr->status = SUCCESS;
	DevXbusStuffXor (infoPtr);
    }

    return (status);
}

/*----------------------------------------------------------------------
 *
 * Dev_XbusXor
 *
 *	Queue up an XOR for the xbus board.  The callback routine will
 *	get called with client data after the XOR completes.  Since the
 *	callback may get called directly from interrupt level, it MUST
 *	follow all guidelines for interrupt-time routines.  It should
 *	also be short.
 *
 *	The bufArrayPtr should point to an array of addresses to XOR.
 *	The first address will be the destination, and the remainder
 *	(there should be numBuffers) will be the sources.
 *
 * Returns:
 *	Standard Sprite return status.
 * Side effects:
 *	none
 *
 *----------------------------------------------------------------------
 */
ENTRY ReturnStatus
Dev_XbusXor (boardId, destBuf, numBufs, bufArrayPtr, bufLen,
	     callbackProc, clientData)
unsigned int	boardId;
unsigned int	destBuf;
unsigned int	numBufs;
unsigned int* 	bufArrayPtr;
unsigned int	bufLen;
void		(*callbackProc)();
ClientData	clientData;
{
    ReturnStatus	status = SUCCESS;
    DevXbusInfo*	infoPtr;

    if (boardId > DEV_XBUS_MAX_BOARDS) {
	status = FAILURE;
	goto xorExit;
    }
    if ((infoPtr = xbusInfo[boardId]) == NULL) {
	status = FAILURE;
	goto xorExit;
    }

    if (devXbusDebug) {
	printf ("Dev_XbusXor: about to do XOR for %s\n", infoPtr->name);
    }
    MASTER_LOCK (&(infoPtr->mutex));
    status = DevXbusXor (infoPtr, destBuf, numBufs, bufArrayPtr, bufLen,
			 callbackProc, clientData);
    MASTER_UNLOCK (&(infoPtr->mutex));
  xorExit:
    if (status != SUCCESS) {
	(callbackProc)(clientData, status);
    }
    return (SUCCESS);
}

/*----------------------------------------------------------------------
 *
 * xorCallback
 *
 *	Called back when an IOControl XOR finishes.
 *
 * Returns:
 *	none
 * Side effects:
 *	Wakes up waiting XOR.
 *
 *----------------------------------------------------------------------
 */
static
void
xorCallback (infoPtr, status)
DevXbusInfo*	infoPtr;
ReturnStatus	status;
{
    if (devXbusDebug) {
	printf ("xorCallback for %s.\n", infoPtr->name);
    }

    MASTER_LOCK (&infoPtr->mutex);
    infoPtr->state &= ~DEV_XBUS_STATE_XOR_TEST;
    MASTER_UNLOCK (&infoPtr->mutex);
    Sync_MasterBroadcast (&xorTestDone);
}

#if 0
/*
 *----------------------------------------------------------------------
 *
 * makeFreeBuffer
 *
 *	Generate a free buffer by recursively splitting a larger buffer.
 *
 * Returns:
 *	none
 * Side effects:
 *	May split larger buffers to get one of the appropriate size.
 *
 *----------------------------------------------------------------------
 */
static
int
makeFreeBuffer (xbusPtr, power)
DevXbusInfo	*xbusPtr;
int		power;
{
    DevXbusFreeMem	*freeMemPtr;

    if (power >= (DEV_XBUS_MALLOC_NUM_SIZES - 1)) {
	/*
	 * Can't "manufacture" new buffers for the largest size.
	 */
	return FALSE;
    } else if (xbusPtr->freeList[power+1] == NULL) {
	if (!makeFreeBuffer (xbusPtr, power+1)) {
	    return FALSE;
	}
	freeMemPtr = xbusPtr->freeList[power+1];
	xbusPtr->freeList[power+1] = xbusPtr->freeList[power+1]->next;
	freeAddr = freeMemPtr->address;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_XbusMalloc
 *
 *	Allocate some portion of XBUS memory and return its xbus address.
 *	The requested size is rounded up to a power of 2 between 4K and
 *	8 MB.
 *
 * Returns:
 *	Address of the section of XBUS memory which was allocated.
 *	-1 if there is no available memory 
 * Side effects:
 *	May allocate kernel memory for record-keeping.
 *
 *----------------------------------------------------------------------
 */
int
Dev_XbusMalloc (boardNum, size)
int	boardNum;
int	size;
{
    int		bufSize = 1 << DEV_XBUS_MALLOC_MIN_SIZE;
    int		powerCount = 0;
    DevXbusInfo*	xbusPtr;
    DevXbusFreeMem*	freePtr;
    DevXbusFreeMem**	freeListPtr;
    int		freeAddr;

    if ((xbusPtr = xbusInfo[boardNum]) == NULL) {
	return -1;
    }

    /*
     * First, figure out how big the request is.
     */
    while (size > bufSize) {
	bufSize <<= 1;
	if (++powerCount > DEV_XBUS_MALLOC_NUM_SIZES) {
	    return -1;
	}
    }

    freeListPtr = &(xbusPtr->freeList[powerCount]);
    MASTER_LOCK (&(xbusPtr->mutex));
    if ((*freeListPtr == NULL) && (!makeFreeBuffer (xbusPtr, powerCount)) {
	    freeAddr = -1;
    } else {
	freePtr = *freeListPtr;
	*freeListPtr = (*freeListPtr)->next;
	freeAddr = freeMemPtr->address;
	freeMemPtr (
	freePtr->next = xbusPtr->freePtrList;
	xbusPtr->freePtrList = freePtr;
    }
    MASTER_UNLOCK (&(xbusPtr->mutex));

    return freeAddr;
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_XbusFree
 *
 *	Free XBUS memory previously allocated by Dev_XbusMalloc.  Note
 *	that this will *NOT* combine adjacent small pieces into larger
 *	pieces; fragmentation isn't dealt with.
 *
 * Returns:
 *	none
 * Side effects:
 *	May allocate kernel memory for record-keeping.
 *
 *----------------------------------------------------------------------
 */
Dev_XbusFree (boardNum, size, addr)
int	boardNum;
int	size;
unsigned int addr;
{
}
#endif

/*
 *----------------------------------------------------------------------
 *
 * Dev_XbusHippiBuffer
 *
 *	Stuff an (extent, address) pair into either the HIPPI-S or HIPPI-D
 *	DMA control FIFO on the xbus board.  No locks are used.  This
 *	routine is for testing purposes ONLY.
 *
 * Returns:
 *	Standard Sprite return status.
 * Side effects:
 *	Causes the xbus board to transfer data to or from the HIPPI
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Dev_XbusHippiBuffer (boardNum, which, size, addr)
int	boardNum;
int	which;		/* >0 means SRC, <0 means DST */
unsigned int	size;
unsigned int	addr;
{
    register DevXbusInfo*	xbusPtr;

    if (devXbusDebug) {
	printf ("Entering Dev_XbusHippiBuffer for board %d.\n", boardNum);
    }

    if ((xbusPtr = xbusInfo[boardNum]) == NULL) {
	printf ("Dev_XbusHippiBuffer: invalid board number (%d).\n", boardNum);
	return DEV_INVALID_UNIT;
    }

    if (which > 0) {
	*(xbusPtr->hippisCtrlFifo) = size >> 2;
	*(xbusPtr->hippisCtrlFifo) = addr >> 2;
    } else if (which < 0) {
	*(xbusPtr->hippidCtrlFifo) = size >> 2;
	*(xbusPtr->hippidCtrlFifo) = addr >> 2;
    } else {
	return (GEN_INVALID_ARG);
    }
    return (SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * DevXbusIOControl
 *
 *	Perform an IO control on the VME link card set.  These can
 *	range from a simple remote ping to playing with the card's
 *	registers such as address modifiers or window selection.
 *	No attempt is made to coordinate between two processes that
 *	mess up the same VME link; that may come in a future version.
 *
 * Results:
 *	
 *
 * Side effects:
 *	May map additional VME address space into the kernel.
 *	Other side effects depending on the ioctl call requested.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ENTRY ReturnStatus
DevXbusIOControl (devicePtr, ioctlPtr, replyPtr)
    Fs_Device *devicePtr;		/* Information about device. */
    register Fs_IOCParam *ioctlPtr;	/* Parameter information (buffer sizes
					 * etc.). */
    register Fs_IOReply *replyPtr;	/* Place to store result information.*/
{
    DevXbusCtrlRegs	*regPtr;
    DevXbusInfo		*infoPtr;
    int			inSize, outSize;
    ReturnStatus	status = SUCCESS;
    ReturnStatus	fmtStatus;
    unsigned int	bufArray[DEV_XBUS_MAX_XOR_BUFS + 3];

    if ((infoPtr = (DevXbusInfo*)devicePtr->data) == NULL) {
	return (DEV_INVALID_UNIT);
    }
    if (devXbusDebug) {
	printf ("DevXbusIOControl: %s doing IOControl 0x%x\n",
		infoPtr->name, ioctlPtr->command);
    }

    regPtr = infoPtr->regs;

    switch (ioctlPtr->command) {
      case IOC_XBUS_RESET:
	status = DevXbusResetBoard (infoPtr);
	break;
      case IOC_XBUS_DEBUG_ON:
	devXbusDebug = TRUE;
	break;
      case IOC_XBUS_DEBUG_OFF:
	devXbusDebug = FALSE;
	break;

      case IOC_XBUS_READ_REG:
      case IOC_XBUS_WRITE_REG:
	{
	    DevXbusRegisterAccess acc;
	    inSize = ioctlPtr->inBufSize;
	    outSize = sizeof (acc);
	    fmtStatus = Fmt_Convert ("ww", ioctlPtr->format, &inSize,
				     ioctlPtr->inBuffer, mach_Format,
				     &outSize, (Address)&acc);
	    if ((fmtStatus != FMT_OK) ||
		(ioctlPtr->outBufSize < sizeof (acc))) {
		printf ("Format of xbus register access failed, 0x%x\n",
			fmtStatus);
		status = GEN_INVALID_ARG;
		goto ioctlExit;
	    }
	    status = accessXbusRegister (infoPtr,acc.registerNum,&(acc.value),
					 ioctlPtr->command);
	    inSize = outSize = sizeof (acc);
	    fmtStatus = Fmt_Convert ("ww", mach_Format, &inSize,
				     (Address)&acc, ioctlPtr->format,
				     &outSize, (Address)ioctlPtr->outBuffer);
	    if (fmtStatus != FMT_OK) {
		printf ("Format of xbus register access output failed, 0x%x\n",
			fmtStatus);
		status = GEN_INVALID_ARG;
		goto ioctlExit;
	    }
	}
	break;

      case IOC_XBUS_DO_XOR:
	{
	    int			numBufs;

	    inSize = ioctlPtr->inBufSize;
	    outSize = sizeof (numBufs);
	    fmtStatus = Fmt_Convert ("w", ioctlPtr->format, &inSize,
				     ioctlPtr->inBuffer, mach_Format,
				     &outSize, (Address)&numBufs);
	    if (fmtStatus != FMT_OK) {
		printf ("Format of IOC_XBUS_DO_XOR size field failed, 0x%x\n",
			fmtStatus);
		status = GEN_INVALID_ARG;
		goto ioctlExit;
	    }
	    if (numBufs > DEV_XBUS_MAX_XOR_BUFS) {
		status = GEN_INVALID_ARG;
		goto ioctlExit;
	    }
	    inSize = ioctlPtr->inBufSize - sizeof (numBufs);
	    outSize = (numBufs + 2) * sizeof (bufArray[0]);
	    fmtStatus = Fmt_Convert ("www*", ioctlPtr->format, &inSize,
				     (char*)ioctlPtr->inBuffer+sizeof(numBufs),
				     mach_Format, &outSize,
				     (Address)&(bufArray[0]));
	    if (fmtStatus != FMT_OK) {
		printf ("Format of IOC_XBUS_DO_XOR parms failed, 0x%x\n",
			fmtStatus);
		status = GEN_INVALID_ARG;
		goto ioctlExit;
	    }
	    if (devXbusDebug) {
		int i;
		printf ("%s doing XOR of: %d buffers, size=0x%x dst=0x%x\n",
			infoPtr->name, numBufs, bufArray[0], bufArray[1]);
		for (i = 0; i < numBufs; i++) {
		    printf ("%s: src buffer %d is 0x%x\n", infoPtr->name, i,
			    bufArray[i+2]);
		}
		printf ("IOC_XBUS_DO_XOR: %s queueing XOR.\n", infoPtr->name);
	    }
	    MASTER_LOCK (&(infoPtr->mutex));
	    infoPtr->state |= DEV_XBUS_STATE_XOR_TEST;
	    DevXbusXor (infoPtr, bufArray[1], numBufs, &(bufArray[2]),
			bufArray[0], xorCallback, (ClientData)infoPtr);
	    if (devXbusDebug) {
		printf ("IOC_XBUS_DO_XOR: %s waiting for completion.\n",
			infoPtr->name);
	    }
	    Sync_MasterWait (&xorTestDone,&(infoPtr->mutex),TRUE);
	    MASTER_UNLOCK (&(infoPtr->mutex));
	    if (devXbusDebug) {
		printf ("IOC_XBUS_DO_XOR: %s done.\n", infoPtr->name);
	    }
	}
	break;

      case IOC_XBUS_TEST_START:
	{
	    unsigned int	testArgs[5];

	    if (infoPtr->state & DEV_XBUS_STATE_TESTING) {
		status = DEV_BUSY;
		goto ioctlExit;
	    }
	    infoPtr->state |= DEV_XBUS_STATE_TESTING;
	    inSize = ioctlPtr->inBufSize;
	    outSize = sizeof (testArgs);
	    fmtStatus = Fmt_Convert ("w5", ioctlPtr->format, &inSize,
				     ioctlPtr->inBuffer, mach_Format,
				     &outSize, (Address)testArgs);
	    if (fmtStatus != FMT_OK) {
		printf ("Format of IOC_XBUS_TEST_START args failed, 0x%x\n",
			fmtStatus);
		status = GEN_INVALID_ARG;
		goto ioctlExit;
	    }
	    if (devXbusDebug) {
		printf ("%s: starting test, args=0x%x 0x%x 0x%x 0x%x 0x%x\n",
			infoPtr->name, testArgs[0], testArgs[1], testArgs[2],
			testArgs[3], testArgs[4]);
	    }
	    DevXbusTestStart (testArgs[0], testArgs[1], testArgs[2],
			      testArgs[3], testArgs[4]);
	}
	break;
      case IOC_XBUS_TEST_STOP:
	if (!(infoPtr->state & DEV_XBUS_STATE_TESTING)) {
	    status = GEN_FAILURE;
	    goto ioctlExit;
	}
	infoPtr->state &= ~DEV_XBUS_STATE_TESTING;
	if (devXbusDebug) {
	    printf ("IOC_XBUS_TEST_STOP: stopping test for %s.\n",
		    infoPtr->name);
	}
	DevXbusTestStop ();
	break;
      case IOC_XBUS_TEST_STATS:
	{
	    if (devXbusDebug) {
		printf ("IOC_XBUS_TEST_STATS: getting test stats for %s.\n",
			infoPtr->name);
	    }
	    if (ioctlPtr->outBufSize < DEV_XBUS_TEST_MAX_STAT_STR) {
		status = GEN_INVALID_ARG;
		goto ioctlExit;
	    }
		
	    DevXbusTestStat (ioctlPtr->outBuffer);
	}
	break;

      case IOC_XBUS_CHECK_PARITY:
	{
	    unsigned int parityOn;
	    unsigned int oldParity;

	    inSize = ioctlPtr->inBufSize;
	    outSize = sizeof (parityOn);
	    fmtStatus = Fmt_Convert ("w", ioctlPtr->format, &inSize,
				     ioctlPtr->inBuffer, mach_Format,
				     &outSize, (Address)&parityOn);
	    if (fmtStatus != FMT_OK) {
		printf ("Format of IOC_XBUS_CHECK_PARITY args failed, 0x%x\n",
			fmtStatus);
		status = GEN_INVALID_ARG;
		goto ioctlExit;
	    }
	    oldParity =
		((infoPtr->resetValue & DEV_XBUS_RESETREG_CHECK_PARITY) != 0);
	    MASTER_LOCK (&(infoPtr->mutex));
	    if (parityOn) {
		infoPtr->resetValue |= DEV_XBUS_RESETREG_CHECK_PARITY;
		regPtr->reset = regPtr->reset | DEV_XBUS_RESETREG_CHECK_PARITY;
	    } else {
		infoPtr->resetValue &= ~DEV_XBUS_RESETREG_CHECK_PARITY;
		regPtr->reset =regPtr->reset & ~DEV_XBUS_RESETREG_CHECK_PARITY;
	    }
	    MASTER_UNLOCK (&(infoPtr->mutex));
	    if (ioctlPtr->outBufSize >= sizeof (oldParity)) {
		outSize = ioctlPtr->outBufSize;
		inSize = sizeof (oldParity);
		fmtStatus = Fmt_Convert ("w", mach_Format, &inSize,
					 (Address)&oldParity, ioctlPtr->format,
					 &outSize, ioctlPtr->outBuffer);
	    }
	}
	break;
      case IOC_XBUS | 10000:
	{
	    unsigned int i, numToLoop, sreg, rreg, doprint;
	    unsigned int printMask = 0x80000000;
	    bcopy (ioctlPtr->inBuffer, &numToLoop, sizeof (numToLoop));
	    doprint = ((numToLoop & printMask) != 0);
	    numToLoop &= ~printMask;
	    for (i = 0; i < numToLoop; i++) {
		sreg = infoPtr->regs->status;
		rreg = infoPtr->regs->reset;
		if ((sreg == rreg) && doprint) {
		    printf ("%s: status reg = reset reg = 0x%x\n",
			    infoPtr->name, sreg);
		}
	    }
	}
	break;
      default:
	status = GEN_NOT_IMPLEMENTED;
	break;
    }

  ioctlExit:
    return (status);
}

/***********************************************************************
 *
 * DevXbusIntr --
 *
 *	The interrupt handler for the xbus board driver.  Currently,
 *	this just prints a message saying that an interrupt was
 *	received.
 *
 ***********************************************************************
 */
ENTRY Boolean
DevXbusIntr (data)
ClientData	data;
{
    DevXbusInfo*	infoPtr = (DevXbusInfo*)data;
    unsigned int	boardStatus;
    void 		(*callbackProc)();
    ReturnStatus	callStatus;
    ClientData		clientData;
    DevXbusXorInfo*	xorPtr;
    unsigned int	tmp;

    boardStatus = infoPtr->regs->status;
    if (devXbusDebug) {
	printf ("DevXbusIntr: %s got interrupt, status = 0x%x\n",
		infoPtr->name, boardStatus);
    }

    if (boardStatus & DEV_XBUS_STATUS_XOR_INTERRUPT) {
	if (devXbusDebug) {
	    printf ("DevXbusIntr: %d in XOR queue for %s.\n", infoPtr->numInQ,
		    infoPtr->name);
	}
	tmp = infoPtr->regs->reset;
	if ((tmp & 0xb) != 0xb) {
	    /*
	     * Reset register messed up!
	     */
	    infoPtr->regs->reset = infoPtr->resetValue;
	    printf ("%s: DevXbusIntr found bad reset value (0x%x\n)",
		    infoPtr->name, tmp);
	} else {
	    infoPtr->regs->reset = tmp & ~DEV_XBUS_RESETREG_CLEAR_XOR_BIT;
	    infoPtr->regs->reset = tmp | DEV_XBUS_RESETREG_CLEAR_XOR_BIT;
	}
	
	if (infoPtr->numInQ > 0) {
	    MASTER_LOCK (&(infoPtr->mutex));
	    infoPtr->state &= ~DEV_XBUS_STATE_XOR_GOING;
	    xorPtr = infoPtr->qHead;
	    callbackProc = xorPtr->callbackProc;
	    clientData = xorPtr->clientData;
	    callStatus = SUCCESS;
	    infoPtr->qHead += 1;
	    if (infoPtr->qHead == infoPtr->qEnd) {
		infoPtr->qHead = infoPtr->xorQueue;
	    }
	    infoPtr->numInQ -= 1;
	    DevXbusStuffXor (infoPtr);
	    MASTER_UNLOCK (&(infoPtr->mutex));
	    (callbackProc)(clientData, callStatus);
	} else {
	    printf ("DevXbusIntr: %s got XOR intr with none outstanding.\n",
		    infoPtr->name);
	}
    } else if (boardStatus & (DEV_XBUS_STATUS_ATC0_PARITY_ERR ||
			      DEV_XBUS_STATUS_ATC1_PARITY_ERR ||
			      DEV_XBUS_STATUS_ATC2_PARITY_ERR ||
			      DEV_XBUS_STATUS_ATC3_PARITY_ERR ||
			      DEV_XBUS_STATUS_SERVER_PARITY_ERR)) {
	printf ("DevXbusIntr: %s has parity error (status reg = 0x%x)!!\n",
		infoPtr->name, boardStatus);
    } else {
	printf ("DevXbusIntr: %s got unknown interrupt (status reg = 0x%x)\n",
		infoPtr->name, boardStatus);
    }
    return (TRUE);
}

/*----------------------------------------------------------------------
 *
 * DevXbusOpen --
 *
 *	Open the xbus device.  All we want to do is make sure that
 *	the device exists, and to insert an appropriate value into
 *	the devicePtr->data field.
 *
 * Returns:
 *	Standard Sprite return status.
 * Side effects:
 *	Modifies the devicePtr->data field.
 *
 *----------------------------------------------------------------------
 */
ENTRY ReturnStatus
DevXbusOpen (devicePtr, useFlags, notifyToken, flagsPtr)
Fs_Device	*devicePtr;
int		useFlags;
Fs_NotifyToken	notifyToken;
int		*flagsPtr;
{
    if (devXbusDebug) {
	printf ("DevXbusOpen: Trying to open unit %d\n", devicePtr->unit);
    }
    
    if (devicePtr->unit >= DEV_XBUS_MAX_BOARDS) {
	return (DEV_INVALID_UNIT);
    }

    devicePtr->data = (ClientData)(xbusInfo[devicePtr->unit]);

    if (devicePtr->data == NULL) {
	return (DEV_INVALID_UNIT);
    }
    if (devXbusDebug) {
	printf ("DevXbusOpen: Opened device %s successfully (unit %d).\n",
		((DevXbusInfo*)devicePtr->data)->name, devicePtr->unit);
    }

    return (SUCCESS);
}
