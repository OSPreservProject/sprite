/* 
 * devVMElink.c --
 *
 *	Routines used for running the Bit-3 VME-VME card cage link
 *	board set.  These routines are to set up and handle a master
 *	board; a slave interface should require no (software) setup.
 *
 * Copyright 1990 Regents of the University of California
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
#endif /* not lint */

#include "sprite.h"
#include "stdio.h"
#include "mach.h"
#include "dev.h"
#include "devInt.h"
#include "stdlib.h"
#include "sync.h"
#include "vmMach.h"
#include "devVMElink.h"

#include "dbg.h"
static VMELinkInfo *VMEInfo[DEV_VMELINK_MAX_BOARDS];
static Boolean devVMElinkInitted = FALSE;

#ifdef VMELINK_DEBUG
static int devVMElinkDebug = TRUE;
#else
static int devVMElinkDebug = FALSE;
#endif

typedef void (*VoidFunc)();


/*
 * These are the addresses for each card's "window" register access.
 */
unsigned int windowPhysAddr[] = {0xff8e0000,};

    

/*
 *----------------------------------------------------------------------
 *
 * ProbeVMEBoard
 *
 *	See if the VME link board is actually installed by trying to
 *	read the local status register.
 *
 * Results:
 *	TRUE if the local VME link board was found.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static Boolean
ProbeVMEBoard(address)
int address;			/* Alleged controller address */
{
    ReturnStatus isPresent;
    unsigned char status;
    register volatile CtrlRegs *regsPtr = (volatile CtrlRegs *)address;

    /*
     * Get the local status
     */
    
    isPresent = Mach_Probe(sizeof(regsPtr->LocalStatus),
			   (char *)&(regsPtr->LocalStatus), (char *)&status);
    if (isPresent != SUCCESS) {
	printf ("No VME link board found at 0x%x\n", address);
        return (FALSE);
    } else {
	printf ("VME link board found at 0x%x\n", address);
    }
    return(TRUE);
}

/*
 *----------------------------------------------------------------------
 *
 * DevVMElinkReset
 *
 *	Reset the VME link board.  Also set up any necessary registers
 *	with the proper values.
 *
 * Returns:
 *	SUCCESS if reset worked, error code otherwise.
 *
 * Side effects:
 *	Resets the VME link board.  Any currently-running operation is
 *	aborted.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
DevVMElinkReset (linkInfo)
VMELinkInfo *linkInfo;
{
    unsigned char curStatus = DEV_VMELINK_REMOTE_DOWN;
    ReturnStatus status = SUCCESS;
    CtrlRegs *regPtr = linkInfo->regArea;

    MASTER_LOCK (&linkInfo->mutex);

    if (devVMElinkDebug) {
	printf ("DevVMElinkReset: starting reset.\n");
    }

    linkInfo->state = 0;
    linkInfo->position = 0;
    curStatus = regPtr->LocalStatus;
    if (devVMElinkDebug) {
	printf ("DevVMElinkReset: Status was 0x%x\n", curStatus);
    }
    if (status != SUCCESS) {
	printf("DevVMElinkReset: local board not responding.\n");
	goto resetExit;
    }

    regPtr->LocalCmd = DEV_VMELINK_CLEAR_LOCAL_ERRS;
    MACH_DELAY (100000);
    curStatus = regPtr->LocalStatus;

    if (devVMElinkDebug) {
	printf ("DevVMElinkReset: Status is now 0x%x (after errs cleared)\n",
		curStatus);
    }
    /*
     *	Reset the remote card if it's powered on.
     */
    if (!(curStatus & DEV_VMELINK_REMOTE_DOWN)) {
	printf ("DevVMElinkReset: Resetting remote VME link card...\n");
	regPtr->RemoteCmd1 = DEV_VMELINK_REMOTE_RESET;
	if (devVMElinkDebug) {
	    printf ("DevVMElinkReset: Just reset remote board.\n");
	}
	MACH_DELAY(1000000);
	if (devVMElinkDebug) {
	    printf ("After resetting remote, status is 0x%x\n",
		    regPtr->LocalStatus);
	}
	regPtr->LocalIntrVec = (unsigned char)linkInfo->vectorNumber;
    } else {
	printf ("DevVMElinkReset: Remote VME link down; can't reset.\n");
    }
  resetExit:
    if (devVMElinkDebug) {
	printf ("Just before putting in local flags, status is 0x%x\n",
		regPtr->LocalStatus);
    }
    linkInfo->LocalFlags = 0;
    regPtr->LocalCmd = linkInfo->LocalFlags;
    linkInfo->RemoteFlags1 = 0;
    linkInfo->RemoteFlags2 = 0 /*DEV_VMELINK_REMOTE_BLKMODE_DMA*/;
    if (!(curStatus & DEV_VMELINK_REMOTE_DOWN)) {
	if (devVMElinkDebug) {
	    printf ("Just before putting in remote flags, status is 0x%x\n",
		    regPtr->LocalStatus);
	}
	regPtr->RemoteCmd1 = linkInfo->RemoteFlags1;
	regPtr->RemoteCmd2 = linkInfo->RemoteFlags2;
    } else {
	if (devVMElinkDebug) {
	    printf ("DevVMElinkReset: remote down; not initted cmd regs\n");
	}
    }
    MASTER_UNLOCK (&linkInfo->mutex);
    return(status);
}

/***********************************************************************
 *
 * DevVMElinkAttach
 *
 *	Attach a VME link board as a block device.
 *
 * Returns: block device handle
 *
 ***********************************************************************
 */
DevBlockDeviceHandle *
DevVMElinkAttach (devPtr)
Fs_Device *devPtr;
{
    DevVMElinkHandle *handlePtr;
    VMELinkInfo *linkInfo;

    if (devPtr->unit >= DEV_VMELINK_MAX_BOARDS ||
	((devPtr->data = (ClientData)(VMEInfo[devPtr->unit])) == NULL)) {
	return ((DevBlockDeviceHandle *)NIL);
    }

    linkInfo = (VMELinkInfo *)(devPtr->data);
    handlePtr = (DevVMElinkHandle *) malloc (sizeof (DevVMElinkHandle));
    if (handlePtr == NULL) {
	return ((DevBlockDeviceHandle *)NIL);
    }
    handlePtr->blockHandle.blockIOProc = DevVMElinkBlockIO;
    handlePtr->blockHandle.IOControlProc = DevVMElinkBlockIOControl;
    handlePtr->blockHandle.releaseProc = DevVMElinkRelease;
    handlePtr->blockHandle.minTransferUnit = sizeof (int);
    handlePtr->blockHandle.maxTransferSize = DEV_VMELINK_MAX_TRANSFER_SIZE;
    handlePtr->linkInfo = linkInfo;
    handlePtr->magic = DEV_VMELINK_HANDLE_MAGIC;
    MASTER_LOCK (&(linkInfo->mutex));
    linkInfo->numAttached += 1;
    MASTER_UNLOCK (&(linkInfo->mutex));
    return ((DevBlockDeviceHandle *)handlePtr);
}

/***********************************************************************
 *
 * DevVMElinkRelease
 *
 *	Release the resources used by an attach().  Currently, this
 *	means decrementing the count of attachers.
 *
 ***********************************************************************
 */
ReturnStatus
DevVMElinkRelease (devHandle)
DevVMElinkHandle *devHandle;
{
    VMELinkInfo *linkInfo;

    if (devHandle->magic != DEV_VMELINK_HANDLE_MAGIC) {
	printf ("DevVMElinkRelease: bad handle magic #: 0x%x\n",
		devHandle->magic);
	return (FAILURE);
    }
    linkInfo = devHandle->linkInfo;
    MASTER_LOCK (&(linkInfo->mutex));
    linkInfo->numAttached -= 1;
    MASTER_UNLOCK (&(linkInfo->mutex));
    free ((char *)devHandle);

    return (SUCCESS);
}

/***********************************************************************
 *
 * DevVMElinkBlockIO
 *
 *	This routine copies all the information into a VME transfer
 *	request block.  It then queues up the I/O by calling DevQueue.
 *
 * Returns: standard Sprite return status
 *
 * Side effects: Queues up a request for a VME transfer
 *
 ***********************************************************************
 */
ReturnStatus
DevVMElinkBlockIO (devHandle, devReqPtr)
DevVMElinkHandle *devHandle;
DevBlockDeviceRequest *devReqPtr;
{
    DevVMElinkReq *req;

    req = (DevVMElinkReq *)devReqPtr->ctrlData;
    List_InitElement ((List_Links *)&(req->links));
    req->linkInfo = devHandle->linkInfo;
    req->origReq = devReqPtr;
    req->startAddress = devReqPtr->startAddress;
    req->length = devReqPtr->bufferLen;
    req->buffer = devReqPtr->buffer;
    req->operation = devReqPtr->operation;
    req->status = SUCCESS;

    Dev_QueueInsert (req->linkInfo->reqQueue,(List_Links *)req);

    return (SUCCESS);
}

/***********************************************************************
 *
 * DevVMElinkFinishCopy
 *
 *	This procedure will finish a copy operation started by
 * DevVMElinkXferData.  For large requests, it will be a callback
 * routine from the interrupt handler.  For smaller requests which
 * do not use DMA, it will simply copy the requested data over the
 * link board word by word.  In either case, it will look for pending
 * requests before it exits.  If one is found, it will be started up.
 *
 * Side effects:
 *	Copies data over the VME link.  May affect the device queues by
 *	servicing the next request.
 *
 ***********************************************************************
 */
void
DevVMElinkFinishCopy (reqPtr)
DevVMElinkReq	*reqPtr;
{
    int		size;
    unsigned int remoteAddr;
    Address	localAddr;
    Address	dmaAddr;
    CtrlRegs	*regPtr;
    VMELinkInfo *linkInfo;
    DevVMElinkReq *nextReq;

    if (reqPtr == NULL) {
	panic ("DevVMElinkFinishCopy: no current I/O\n");
    }
    linkInfo = reqPtr->linkInfo;
    regPtr = linkInfo->regArea;
    if (reqPtr->dmaSize > 0) {
	VmMach_32BitDMAFree (reqPtr->dmaSize, (Address)(reqPtr->dmaSpace));
    }

    size = reqPtr->length - reqPtr->dmaSize;
    remoteAddr = (unsigned int)reqPtr->startAddress + reqPtr->dmaSize;
    localAddr = reqPtr->buffer + reqPtr->dmaSize;

    /*
     * Copy the rest of the data.  Use the remote page register to
     * supply the upper 16 bits, and the 64K map space for the
     * rest.
     */
    dmaAddr = (Address)linkInfo->smallMap + (remoteAddr & 0xffff);
    regPtr->RemoteCmd1 = linkInfo->RemoteFlags1 | DEV_VMELINK_USE_PAGE_REG;
    regPtr->RemotePageAddrHigh = ((unsigned int)remoteAddr >> 24) & 0xff;
    regPtr->RemotePageAddrLow =  ((unsigned int)remoteAddr >> 16) & 0xff;
    if (reqPtr->operation == FS_READ) {
	while (size > 0) {
	    *(int *)localAddr = *(int *)dmaAddr;
	    dmaAddr += 4;
	    localAddr += 4;
	    size -= 4;
	    remoteAddr += 4;
	    if (((unsigned int)remoteAddr & 0xffff) == 0) {
		regPtr->RemotePageAddrHigh =
		    ((unsigned int)remoteAddr >> 24) & 0xff;
		regPtr->RemotePageAddrLow =
		    ((unsigned int)remoteAddr >> 16) & 0xff;
	    }
	}
    } else {
	while (size > 0) {
	    *(int *)dmaAddr = *(int *)localAddr;
	    dmaAddr += 4;
	    localAddr += 4;
	    size -= 4;
	    remoteAddr += 4;
	    if (((unsigned int)remoteAddr & 0xffff) == 0) {
		regPtr->RemotePageAddrHigh =
		    ((unsigned int)remoteAddr >> 24) & 0xff;
		regPtr->RemotePageAddrLow =
		    ((unsigned int)remoteAddr >> 16) & 0xff;
	    }
	}
    }

    regPtr->RemoteCmd1 = linkInfo->RemoteFlags1;
    linkInfo->state &= ~DEV_VMELINK_STATE_DMA_IN_USE;
    linkInfo->curReq = NULL;

    /*
     * Check to see if there's another request outstanding.  If there
     * is, start it up.
     */

    MASTER_LOCK (&linkInfo->queueMutex);
    nextReq = (DevVMElinkReq *)Dev_QueueGetNext (linkInfo->reqQueue);
    if (nextReq != (DevVMElinkReq *)NIL) {

	linkInfo->state |= DEV_VMELINK_STATE_DMA_IN_USE;
	Proc_CallFunc ((VoidFunc)DevVMElinkXferData, (ClientData)nextReq, 0);
    }
    MASTER_UNLOCK (&linkInfo->queueMutex);

    (reqPtr->doneProc)(reqPtr->origReq, reqPtr->status, reqPtr->length);
}

/*
 *----------------------------------------------------------------------
 *
 * entryAvailProc
 *
 *	Call DevVMElinkXferData when an element is inserted.
 *
 *----------------------------------------------------------------------
 */
static
Boolean
entryAvailProc (clientData, req)
ClientData clientData;
DevVMElinkReq *req;
{
    VMELinkInfo *linkInfo = req->linkInfo;
    int isBusy;

    MASTER_LOCK (&linkInfo->mutex);
    isBusy = linkInfo->state & DEV_VMELINK_STATE_DMA_IN_USE;
    MASTER_UNLOCK (&linkInfo->mutex);

    if (isBusy) {
	return (FALSE);
    }

    return (DevVMElinkXferData (req));
}

/*
 *----------------------------------------------------------------------
 *
 * DevVMElinkXferData
 *
 *	Transfer data over the VME link.  If there is a small amount
 *	of data, just copy it over word by word.  If the amount is
 *	larger than 256, copy most of it by DMA and the remainder
 *	word by word.  NOTE: the transfer must be word-aligned on
 *	both sides.  If not, the routine returns an error.
 *
 *	Since the link can't be multiplexed, transfers queue up
 *	at the start of this routine.
 *	
 * Returns:
 *	TRUE if the request was processed; FALSE otherwise.
 *
 * Side effects:
 *	Data is transferred over the VME link boards.
 *
 *----------------------------------------------------------------------
 */
Boolean
DevVMElinkXferData (reqPtr)
DevVMElinkReq	*reqPtr;
{
    register VMELinkInfo *linkData = reqPtr->linkInfo;
    int bufSize;
    unsigned int remoteAddr = reqPtr->startAddress;
    unsigned int bufAddr = (unsigned int)reqPtr->buffer;
    int dmaSize = 0;
    unsigned long dmaAddr;
    unsigned char dmaCmd;
    DmaRegs *dmaPtr = linkData->dmaRegs;

    linkData->curReq = reqPtr;
    bufSize = reqPtr->length;
    if (devVMElinkDebug) {
	printf ("VMElink:  Trying I/O at 0x%x for %x bytes.\n",
		remoteAddr, bufSize);
    }

    if (bufSize >= linkData->minDmaSize) {
	/*
	 * Map in buffer for DMA, and copy in/out a multiple of
	 * 256 bytes.  The remainder will be copied over later.
	 */
	dmaSize = bufSize & DEV_VMELINK_DMA_BUFSIZE_MASK;
	dmaAddr = (unsigned int)VmMach_32BitDMAAlloc(dmaSize,(Address)bufAddr);
	if (dmaAddr == 0) {
	    panic ("DevVMElinkXferData: couldn't allocate DMA space.\n");
	}
	reqPtr->dmaSpace = (Address)dmaAddr;
	reqPtr->dmaSize = dmaSize;

	/*
	 * gets user address modifier.
	 * requires addr mod 0x0d on link board
	 * Convert dmaAddr to a VME address.
	 */
	dmaPtr->localDmaAddr3 = (dmaAddr & 0xff000000) >> 24;
	dmaPtr->localDmaAddr2 = (dmaAddr & 0x00ff0000) >> 16;
	dmaPtr->localDmaAddr1 = (dmaAddr & 0x0000ff00) >> 8;
	dmaPtr->localDmaAddr0 = (dmaAddr & 0x000000ff);
	dmaPtr->dmaLength2 = (dmaSize & 0x00ff0000) >> 16;
	dmaPtr->dmaLength1 = (dmaSize & 0x0000ff00) >> 8;
	dmaPtr->remoteDmaAddr3 = (remoteAddr & 0xff000000) >> 24;
	dmaPtr->remoteDmaAddr2 = (remoteAddr & 0x00ff0000) >> 16;
	dmaPtr->remoteDmaAddr1 = (remoteAddr & 0x0000ff00) >> 8;
	dmaPtr->remoteDmaAddr0 = (remoteAddr & 0x000000ff);
	linkData->state |= DEV_VMELINK_STATE_DMA_IN_USE;
	dmaCmd = DEV_VMELINK_DMA_START | DEV_VMELINK_DMA_LONGWORD |
	    DEV_VMELINK_DMA_LOCAL_PAUSE | DEV_VMELINK_DMA_ENABLE_INTR |
		DEV_VMELINK_DMA_BLOCK_MODE;
	dmaCmd |= (reqPtr->operation == FS_WRITE) ?
	    DEV_VMELINK_DMA_LOCAL_TO_REMOTE : DEV_VMELINK_DMA_REMOTE_TO_LOCAL;
	dmaPtr->localDmaCmdReg = dmaCmd;
	/*
	 * DevVMElinkFinishCopy will be called after the interrupt is taken.
	 * It will also check to see if there are more entries waiting in
	 * the device queue.
	 */
    } else {
	reqPtr->dmaSpace = (Address)NIL;
	reqPtr->dmaSize = 0;
	DevVMElinkFinishCopy (reqPtr);
    }
    return (TRUE);
}

/*----------------------------------------------------------------------
 *
 * DevVMElinkIntr
 *
 *	Called when an interrupt from the link card is received.  This
 *	should only happen when a DMA operation finishes.  Interrupts
 *	from a remote card cage should be handled in the (separate)
 *	device driver for the remote device.
 *
 * Results:
 *	TRUE if this link card caused the interrupt.
 *
 * Side effects:
 *	Notifies any waiting processes, and clears the interrupt bits
 *	on the link card.
 *
 *----------------------------------------------------------------------
 */
Boolean
DevVMElinkIntr (data)
ClientData data;
{
    VMELinkInfo *linkInfo = (VMELinkInfo *)data;
    DmaRegs *dmaRegs;
    Boolean retVal = FALSE;
    unsigned char tmpReg;

    dmaRegs = linkInfo->dmaRegs;
    MASTER_LOCK (&linkInfo->mutex);
    tmpReg = dmaRegs->localDmaCmdReg;
    if (tmpReg & DEV_VMELINK_DMA_DONE) {
	tmpReg &= ~(DEV_VMELINK_DMA_DONE & DEV_VMELINK_DMA_START);
	dmaRegs->localDmaCmdReg = tmpReg;
	if (!(linkInfo->state & DEV_VMELINK_STATE_DMA_IN_USE)) {
	    if (devVMElinkDebug) {
		printf ("DevVMElinkIntr: DMA finished but shouldn't have\n");
	    }
	    if (linkInfo->curReq != NULL) {
		linkInfo->curReq->status = FAILURE;
	    }
	}
	/*
	 * Callback DevVMElinkFinishCopy to copy the remainder of
	 * the data.
	 */
	Proc_CallFunc ((VoidFunc)DevVMElinkFinishCopy,
		       (ClientData)linkInfo->curReq, 0);
	retVal = TRUE;
    }

    MASTER_UNLOCK (&linkInfo->mutex);
    return (retVal);
}

/***********************************************************************
 *
 * setRemotePage
 *
 *	Set the bits which supply the remote page address to those
 *	passed.  Turn on the flag which tells the link board to use
 *	those bits.
 *
 ***********************************************************************
 */
static
void
setRemotePage (linkInfo, pageNum)
register VMELinkInfo	*linkInfo;
unsigned int		pageNum;
{
    register CtrlRegs	*regPtr = linkInfo->regArea;

    if (devVMElinkDebug) {
	printf ("Setting remote VME page to 0x%04x\n", pageNum);
    }
    MASTER_LOCK (&linkInfo->mutex);
    linkInfo->RemoteFlags1 |= DEV_VMELINK_USE_PAGE_REG;
    regPtr->RemoteCmd1 = linkInfo->RemoteFlags1;
    regPtr->RemotePageAddrHigh = (unsigned char)((pageNum >> 8) & 0xff);
    regPtr->RemotePageAddrLow = (unsigned char)(pageNum & 0xff);
    linkInfo->state |= DEV_VMELINK_STATE_PAGE_MODE;
    if (devVMElinkDebug) {
	printf ("Page set to 0x%02x%02x; flags are 0x%x\n",
		regPtr->RemotePageAddrHigh, regPtr->RemotePageAddrLow,
		linkInfo->RemoteFlags1);
    }
    MASTER_UNLOCK (&linkInfo->mutex);
}

/***********************************************************************
 *
 * turnOffPageMode
 *
 *	Turn off page mode as established using setRemotePage.
 *
 ***********************************************************************
 */
static
void
turnOffPageMode (linkInfo)
register VMELinkInfo	*linkInfo;
{
    MASTER_LOCK (&linkInfo->mutex);
    linkInfo->RemoteFlags1 &= ~DEV_VMELINK_USE_PAGE_REG;
    linkInfo->regArea->RemoteCmd1 = linkInfo->RemoteFlags1;
    linkInfo->state &= ~DEV_VMELINK_STATE_PAGE_MODE;
    MASTER_UNLOCK (&linkInfo->mutex);
}

/***********************************************************************
 *
 * DevVMElinkAccessRemoteMemory --
 *
 *	Access memory across the link board.  The accesses are limited
 *	to 4000 bytes, since that's all an ioctl will copy in or out
 *	(with the necessary header).  The bytes must all be consecutive,
 *	and must be long-word sized and aligned.
 *
 ***********************************************************************
 */
ReturnStatus
DevVMElinkAccessRemoteMemory (ioctlPtr, linkInfo)
register Fs_IOCParam *ioctlPtr;
register VMELinkInfo *linkInfo;
{
    int			inSize, outSize;
    int			fmtStatus;
    ReturnStatus	status = SUCCESS;
    DevVMElinkAccessMem	memAccess;
    unsigned int	buf[1000];
    unsigned int	*remote, *local;
    unsigned int	remotePage, remoteOffsetInPage;
    int			nbytes;


    inSize = ioctlPtr->inBufSize;
    outSize = sizeof (memAccess) - sizeof (memAccess.data);
    fmtStatus = Fmt_Convert ("www", ioctlPtr->format, &inSize,
			     ioctlPtr->inBuffer, mach_Format, &outSize,
			     (Address)&memAccess);
    if (fmtStatus != FMT_OK) {
	printf ("DevVMElinkAccessRemoteMemory:cmd format failed, 0x%x\n",
		fmtStatus);
	status = GEN_INVALID_ARG;
	goto accessExit;
    }
    if (memAccess.size > sizeof (buf)) {
	printf ("DevVMElinkAccessRemoteMemory: too much data (0x%x bytes)\n",
		memAccess.size);
	goto accessExit;
    }

    if (devVMElinkDebug) {
	printf ("DevVMElinkAccessRemoteMemory: trying 0x%x\n",
		memAccess.destAddress);
	printf ("DevVMElinkAccessRemoteMemory: map at virtual 0x%x\n",
		linkInfo->smallMap);
    }
    remoteOffsetInPage = (((unsigned int)memAccess.destAddress) & 0xffff);
    if ((remoteOffsetInPage & 0x3) != 0) {
	printf ("DevVMElinkAccessRemoteMemory: remote addr (0x%x) unaligned\n",
		memAccess.destAddress);
	goto accessExit;
    }
    remotePage = ((unsigned int)memAccess.destAddress >> 16) & 0xffff;
    remote = (unsigned int *)(linkInfo->smallMap + remoteOffsetInPage);
    setRemotePage (linkInfo, remotePage);
    local = buf;
    if (devVMElinkDebug) {
	printf ("DevVMELinkAccessRemoteMemory: local 0x%x, remote 0x%x\n",
		local, remote);
	printf ("DevVMELinkAccessRemoteMemory: page offset 0x%x\n",
		remoteOffsetInPage);
    }

    if (memAccess.direction == DEV_VMELINK_TO_REMOTE) {
	inSize = memAccess.size;
	outSize = sizeof (buf);
	fmtStatus = Fmt_Convert
	    ("w*", ioctlPtr->format, &inSize,
	     (Address)(((DevVMElinkAccessMem*)(ioctlPtr->inBuffer))->data),
	     mach_Format, &outSize, (Address)buf);
	if (fmtStatus != FMT_OK) {
	    printf ("DevVMElinkAccessRemoteMemory:data format failed, 0x%x\n",
		    fmtStatus);
	    status = GEN_INVALID_ARG;
	    goto accessExit;
	}
	for (nbytes = 0; nbytes < memAccess.size; nbytes += sizeof (int)) {
	    *(remote++) = *(local++);
	    remoteOffsetInPage += sizeof (int);
	    if ((remoteOffsetInPage & 0xffff) == 0) {
		remote = (unsigned int *)linkInfo->smallMap;
		remotePage += 1;
		setRemotePage (linkInfo, remotePage);
	    }
	}
    } else if (memAccess.direction == DEV_VMELINK_TO_LOCAL) {
	for (nbytes = 0; nbytes < memAccess.size; nbytes += sizeof (int)) {
	    *(local++) = *(remote++);
	    remoteOffsetInPage += sizeof (int);
	    if ((remoteOffsetInPage & 0xffff) == 0) {
		remote = (unsigned int *)linkInfo->smallMap;
		remotePage += 1;
		setRemotePage (linkInfo, remotePage);
	    }
	}
	inSize = memAccess.size;
	outSize = ioctlPtr->outBufSize + sizeof (memAccess.data) -
	    sizeof (memAccess);
	fmtStatus = Fmt_Convert
	    ("w*", mach_Format, &inSize,(Address)buf,ioctlPtr->format,&outSize,
	     (Address)(((DevVMElinkAccessMem*)(ioctlPtr->outBuffer))->data));
	if (fmtStatus != FMT_OK) {
	    printf ("DevVMElinkAccessRemoteMemory:data format failed, 0x%x\n",
		    fmtStatus);
	    status = GEN_INVALID_ARG;
	    goto accessExit;
	}
    } else {
	printf ("DevVMElinkAccessRemoteMemory: illegal direction 0x%x\n",
		memAccess.direction);
	status = GEN_INVALID_ARG;
	goto accessExit;
    }

  accessExit:
    turnOffPageMode (linkInfo);

    return (status);
}

/***********************************************************************
 *
 * DevVMElinkBlockIOControl
 *
 *	This is the main IOControl routine for the VMElink driver.
 *	It's easier to fake a "standard" IOControl using the block
 *	driver than vice versa, since the standard IOControl has an
 *	easy way of getting a real handle, but the BlockIOControl
 *	has no way of getting an Fs_Device.
 *
 * Returns: standard Sprite return status
 *
 * Side effects: depends on IOControl
 *
 ***********************************************************************
 */
ENTRY ReturnStatus
DevVMElinkBlockIOControl (handlePtr, ioctlPtr, replyPtr)
DevVMElinkHandle *handlePtr;
register Fs_IOCParam *ioctlPtr;
register Fs_IOReply *replyPtr;

/* ARGSUSED */
{
    ReturnStatus status = SUCCESS;
    register volatile CtrlRegs *regPtr;
    register VMELinkInfo *linkData;
    unsigned int passedData;
    DevVMElinkStatus *boardStatus;
    int inSize, outSize;
    int fmtStatus;

    if (devVMElinkDebug) {
	printf ("VMElink: doing IOControl 0x%x\n", ioctlPtr->command);
    }

    linkData = handlePtr->linkInfo;
    regPtr = linkData->regArea;

    /*
     * If the remote board isn't working, don't even bother with the
     * IOcontrol call and just return failure.
     */
    if ((ioctlPtr->command != IOC_VMELINK_DEBUG_ON) &&
	(ioctlPtr->command != IOC_VMELINK_DEBUG_OFF) &&
	(regPtr->RemoteCmd1 & DEV_VMELINK_REMOTE_DOWN)) {
	return (DEV_OFFLINE);
    }

    switch (ioctlPtr->command) {
      case IOC_VMELINK_STATUS:
	boardStatus  =(DevVMElinkStatus *)ioctlPtr->inBuffer;
	boardStatus->LocalStatus = (int)regPtr->LocalStatus;
	boardStatus->RemoteStatus = (int)regPtr->RemoteCmd1;
	break;
      case IOC_VMELINK_SET_ADDRMOD:
	passedData = *(unsigned int *)ioctlPtr->inBuffer;
	regPtr->RemoteAddrMod = (unsigned char)passedData;
	linkData->RemoteFlags2 |= DEV_VMELINK_REMOTE_USE_ADDRMOD;
	regPtr->RemoteCmd2 = linkData->RemoteFlags2;
	break;
      case IOC_VMELINK_NO_ADDRMOD:
	linkData->RemoteFlags2 &= ~DEV_VMELINK_REMOTE_USE_ADDRMOD;
	regPtr->RemoteCmd2 = linkData->RemoteFlags2;
	break;
      case IOC_VMELINK_LOW_VME:
	linkData->addrMsb = 0x00000000;
	break;
      case IOC_VMELINK_HIGH_VME:
	linkData->addrMsb = 0x80000000;
	break;
      case IOC_VMELINK_DEBUG_ON:
	devVMElinkDebug = TRUE;
	break;
      case IOC_VMELINK_DEBUG_OFF:
	devVMElinkDebug = FALSE;
	break;
      case IOC_VMELINK_RESET:
	status = DevVMElinkReset (linkData);
	break;
      case IOC_VMELINK_READ_BOARD_STATUS:
	{
	    unsigned char	regs[0x20];
	    
	    regs[0x1] = regPtr->LocalCmd;
	    regs[0x3] = regPtr->LocalStatus;
	    regs[0x9] = regPtr->RemoteCmd1;
	    regs[0x8] = regPtr->RemoteCmd2;
	    regs[0x10] = linkData->dmaRegs->localDmaCmdReg;
	    
	    inSize = sizeof (regs);
	    outSize = ioctlPtr->outBufSize;
	    fmtStatus = Fmt_Convert ("b32", mach_Format, &inSize,
				     (Address)regs, ioctlPtr->format, &outSize,
				     (Address)ioctlPtr->outBuffer);
	    if (fmtStatus != FMT_OK) {
		printf ("Format of VMELINK_READ_BOARD_STATUS failed, 0x%x\n",
			fmtStatus);
		return GEN_INVALID_ARG;
	    }
	}
	break;
      case IOC_VMELINK_ACCESS_REMOTE_MEMORY:
	status = DevVMElinkAccessRemoteMemory (ioctlPtr, linkData);
	break;

      case IOC_VMELINK_WRITE_REG:
	{
	    unsigned char stuff[2];

	    inSize = ioctlPtr->inBufSize;
	    outSize = sizeof (stuff);
	    fmtStatus = Fmt_Convert ("b2", ioctlPtr->format, &inSize,
				     (Address)ioctlPtr->inBuffer, mach_Format,
				     &outSize, (Address)stuff);
	    if (fmtStatus != FMT_OK) {
		printf ("Format of VMELINK_WRITE_REG failed, 0x%x\n",
			fmtStatus);
		return GEN_INVALID_ARG;
	    }
	    status = Mach_Probe (sizeof (stuff[1]), (Address)&(stuff[1]),
				 (Address)regPtr + stuff[0]);
	}
	break;
      case IOC_VMELINK_READ_REG:
	{
	    unsigned char stuff[2];

	    inSize = ioctlPtr->inBufSize;
	    outSize = sizeof (stuff);
	    fmtStatus = Fmt_Convert ("b2", ioctlPtr->format, &inSize,
				     (Address)ioctlPtr->inBuffer, mach_Format,
				     &outSize, (Address)stuff);
	    if (fmtStatus != FMT_OK) {
		printf ("Format of VMELINK_READ_REG failed, 0x%x\n",fmtStatus);
		return GEN_INVALID_ARG;
	    }
	    status = Mach_Probe (sizeof (stuff[1]), (Address)regPtr + stuff[0],
				 (Address)&(stuff[1]));
	    inSize = sizeof (stuff);
	    outSize = ioctlPtr->outBufSize;
	    fmtStatus = Fmt_Convert ("b2", mach_Format, &inSize,(Address)stuff,
				     ioctlPtr->format, &outSize,
				     (Address)ioctlPtr->outBuffer);
	    if (fmtStatus != FMT_OK) {
		printf ("Format of VMELINK_READ_REG failed, 0x%x\n",fmtStatus);
		return GEN_INVALID_ARG;
	    }
	}
	break;

      case IOC_VMELINK_PING_REMOTE:
	/*
	 * Do nothing here; the null call will tell whether the remote
	 * board has been turned on or not.
	 */
	break;
      case IOC_LOCK:
      case IOC_UNLOCK:
	return (GEN_NOT_IMPLEMENTED);
	break;
#if 0
      case IOC_REPOSITION:
	{
	    Ioc_RepositionArgs pos;
	    unsigned int curAddr;
	    inSize = ioctlPtr->inBufSize;
	    outsize = sizeof (pos);
	    fmtStatus = Fmt_Convert ("ww", ioctlPtr->format, &inSize,
				     ioctlPtr->inBuffer, mach_Format,
				     &outSize, &pos);
	    if (fmtStatus != FMT_OK) {
		printf ("DevVMElinkIOControl: IOC 0x%x error 0x%x\n",	
			ioctlPtr->command, fmtStatus);
		return (GEN_INVALID_ARG);
	    }
	    switch (pos.base) {
	      case IOC_BASE_ZERO:
		curAddr = 0;
		break;
	      case IOC_BASE_CURRENT:
		curAddr = linkData->position;
		break;
	      case IOC_BASE_EOF:
		curAddr = 0xffffffff;
		break;
	      default:
		return (GEN_INVALID_ARG);
		break;
	    }
	    if (pos.offset < 0) {
		linkData->position = curAddr - (unsigned int)(-pos.offset);
	    } else {
		linkData->position = curAddr + (unsigned int)pos.offset;
	    }
	}
	break;
#endif
      case IOC_GET_FLAGS:
      case IOC_SET_FLAGS:
      case IOC_SET_BITS:
      case IOC_CLEAR_BITS:
	break;
      default:
	return (GEN_NOT_IMPLEMENTED);
	break;
    }

    return (status);
}

/*
 *----------------------------------------------------------------------
 *
 * DevVMElinkIOControl
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
ENTRY ReturnStatus
DevVMElinkIOControl (devicePtr, ioctlPtr, replyPtr)
Fs_Device *devicePtr;	/* Information about device. */
Fs_IOCParam *ioctlPtr;	/* Parameter information (buffer sizes etc.). */
Fs_IOReply *replyPtr;	/* Place to store result information.*/
{
    VMELinkInfo *linkInfo;

    if ((devicePtr->unit >= DEV_VMELINK_MAX_BOARDS) ||
	((linkInfo = VMEInfo[devicePtr->unit]) == NULL)) {
	return (DEV_INVALID_UNIT);
    }
    return (DevVMElinkBlockIOControl(&(linkInfo->handle), ioctlPtr, replyPtr));
}

/*
 *----------------------------------------------------------------------
 *
 * DevVMElinkInit
 *
 *	Initialize the VME link board (assuming that there is one
 *	installed).
 *	
 *
 * Results:
 *	Returns the "device number" of the VME link.
 *
 * Side effects:
 *	May map additional VME address space into the kernel.
 *
 *----------------------------------------------------------------------
 */
ENTRY
ClientData
DevVMElinkInit(cntrlPtr)
    DevConfigController *cntrlPtr;
{
    CtrlRegs *regPtr = (CtrlRegs *)cntrlPtr->address;
    register VMELinkInfo *linkInfo;
    char semName[40];
    int linkNum;
    ReturnStatus status;
    DevBlockDeviceHandle *blockHandle;
    Address	lastAddr;
    Address	curMapAddr, newAddr;
    int		i;

    if (!devVMElinkInitted) {
	int i;
	for (i = 0; i < DEV_VMELINK_MAX_BOARDS; i++) {
	    VMEInfo[i] = NULL;
	}
	devVMElinkInitted = TRUE;
    }

    /*
     * If the VME link board isn't installed or there are too many boards,
     * just return.
     */
    linkNum = cntrlPtr->controllerID;
    if (linkNum >= DEV_VMELINK_MAX_BOARDS ||
	ProbeVMEBoard (regPtr) == FALSE) {
	return (DEV_NO_CONTROLLER);
    }
    linkInfo = VMEInfo[linkNum] = (VMELinkInfo *)malloc (sizeof (VMELinkInfo));
    linkInfo->unit = linkNum;
    linkInfo->LocalFlags = 0;
    linkInfo->RemoteFlags1 = 0;
    linkInfo->RemoteFlags2 = 0 /*DEV_VMELINK_REMOTE_BLKMODE_DMA*/;
    linkInfo->regArea = regPtr;
    linkInfo->dmaRegs = (DmaRegs *)((char *)regPtr + sizeof (CtrlRegs));
    linkInfo->addrMsb = 0x80000000;
    linkInfo->state = 0;
    linkInfo->vectorNumber = cntrlPtr->vectorNumber;
    linkInfo->minDmaSize = DEV_VMELINK_MIN_DMA_SIZE;
    linkInfo->position = 0;

    sprintf (semName, "VME link 0x%x", linkNum);
    Sync_SemInitDynamic (&(linkInfo->mutex), semName);

    sprintf (semName, "VME link queues 0x%x", linkNum);
    Sync_SemInitDynamic (&(linkInfo->queueMutex), semName);
    linkInfo->ctrlQueues =
	Dev_CtrlQueuesCreate (&(linkInfo->queueMutex), entryAvailProc);
    linkInfo->reqQueue =
	Dev_QueueCreate (linkInfo->ctrlQueues, DEV_QUEUE_FIFO_INSERT, 0, 0);

    linkInfo->numAttached = 0;
    linkInfo->curReq = NULL;
    blockHandle = &(linkInfo->handle.blockHandle);
    linkInfo->handle.linkInfo = linkInfo;
    blockHandle->blockIOProc = DevVMElinkBlockIO;
    blockHandle->IOControlProc = DevVMElinkBlockIOControl;
    blockHandle->releaseProc = DevVMElinkRelease;
    blockHandle->minTransferUnit = sizeof (int);
    blockHandle->maxTransferSize = DEV_VMELINK_MAX_TRANSFER_SIZE;

    /*
     * Map in a 64K window so we can use the page register to read or
     * write from any VME address in the remote cage.
     */
#if 0
    linkInfo->smallMap = (Address)VmMach_MapInDevicePages
	(windowPhysAddr[linkNum],0x10000,DEV_VME_D32A24);
    if (linkInfo->smallMap == NULL) {
	return (DEV_NO_CONTROLLER);
    }
#else
    curMapAddr = (Address)windowPhysAddr[linkNum];
    linkInfo->smallMap =(Address)VmMach_MapInDevice(curMapAddr,DEV_VME_D32A24);
    lastAddr = linkInfo->smallMap;
    for (i = 0; i < 7; i++) {
	curMapAddr += 0x2000;
	lastAddr += 0x2000;
	newAddr = (Address)VmMach_MapInDevice (curMapAddr, DEV_VME_D32A24);
	if (devVMElinkDebug) {
	    printf ("DevVMElinkInit: mapped VME 0x%x to virtual 0x%x\n",
		    curMapAddr, newAddr);
	}
	if (newAddr != lastAddr) {
	    printf ("DevVMElinkInit: couldn't get enough map space.\n");
	    return (DEV_NO_CONTROLLER);
	}
    }
#endif
    printf ("DevVMElinkInit: window mapped into 0x%x\n", linkInfo->smallMap);

    status = DevVMElinkReset (linkInfo);

    return ((ClientData)(linkInfo));
}

/*
 *----------------------------------------------------------------------
 *
 * DevVMElinkOpen
 *
 *	Open the VME link device.  Since there isn't much to keep
 *	track of, just return a handle for future operations.  The
 *	handle is merely the unit number.
 *	
 * Results:
 *	Standard Sprite ReturnStatus.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ENTRY ReturnStatus
DevVMElinkOpen (devicePtr, useFlags, notifyToken)
    Fs_Device *devicePtr;
    int useFlags;
    Fs_NotifyToken notifyToken;
{
    if (devVMElinkDebug) {
	printf ("DevVMElinkOpen: Trying to open unit %d\n", devicePtr->unit);
    }
    
    if (devicePtr->unit >= DEV_VMELINK_MAX_BOARDS) {
	return (DEV_INVALID_UNIT);
    }

    devicePtr->data = (ClientData)(VMEInfo[devicePtr->unit]);

    if (devicePtr->data == NULL) {
	return (DEV_INVALID_UNIT);
    }
    if (devVMElinkDebug) {
	printf ("VMElink: Opened device successfully (unit %d).\n",
		devicePtr->unit);
    }

    return (SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * DevVMElinkRead
 *
 *	Read some bytes from the remote VME into a buffer provided.
 *	Since this may require playing with the virtual->physical
 *	mapping, the virtual memory may get rearranged.
 *	
 * Results:
 *	SUCCESS if read went OK.
 *
 * Side effects:
 *	May change mapping registers used by the VME driver.
 *
 *----------------------------------------------------------------------
 */

ENTRY ReturnStatus
DevVMElinkRead (devicePtr, readPtr, replyPtr)
Fs_Device *devicePtr;
Fs_IOParam *readPtr;
Fs_IOReply *replyPtr;
{
    DevBlockDeviceRequest blockReq;
    ReturnStatus retval;
    VMELinkInfo *linkInfo = (VMELinkInfo *)devicePtr->data;
    int xferred;

    blockReq.operation = FS_READ;
    blockReq.startAddress = readPtr->offset;
    blockReq.bufferLen = readPtr->length;
    blockReq.buffer = readPtr->buffer;

    retval = Dev_BlockDeviceIOSync
	((DevBlockDeviceHandle *)&(linkInfo->handle), &blockReq, &xferred);
    replyPtr->flags = FS_READABLE | FS_WRITABLE;
    replyPtr->length = xferred;
    return (retval);
}

/*
 *----------------------------------------------------------------------
 *
 * DevVMElinkWrite
 *
 *	Write some bytes from a buffer to the remote VME.
 *	Since this may require playing with the virtual->physical
 *	mapping, the virtual memory may get rearranged.
 *	
 * Results:
 *	SUCCESS if write went OK.
 *
 * Side effects:
 *	May change mapping registers used by the VME driver.
 *
 *----------------------------------------------------------------------
 */
ENTRY ReturnStatus
DevVMElinkWrite (devicePtr, writePtr, replyPtr)
Fs_Device *devicePtr;
Fs_IOParam *writePtr;
Fs_IOReply *replyPtr;
{
    DevBlockDeviceRequest blockReq;
    ReturnStatus retval;
    VMELinkInfo *linkInfo = (VMELinkInfo *)devicePtr->data;
    int xferred;

    blockReq.operation = FS_WRITE;
    blockReq.startAddress = writePtr->offset;
    blockReq.bufferLen = writePtr->length;
    blockReq.buffer = writePtr->buffer;

    retval = Dev_BlockDeviceIOSync
	((DevBlockDeviceHandle *)&(linkInfo->handle), &blockReq, &xferred);
    replyPtr->flags = FS_READABLE | FS_WRITABLE;
    replyPtr->length = xferred;
    return (retval);
}
