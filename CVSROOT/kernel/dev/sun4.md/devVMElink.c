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
#include "mach.h"
#include "dev.h"
#include "devInt.h"
#include "stdlib.h"
#include "devVMElink.h"

#include "dbg.h"

#define XFER_TO_REMOTE		1
#define XFER_FROM_REMOTE	0

/*
 *	This is the control register structure for the VME link
 *	boards.  All of the rsvX fields are listed as reserved in
 *	the manual.
 *
 */
typedef struct CtrlRegs {
    unsigned char rsv0;
    unsigned char LocalCmd;		/* local command register */
    unsigned char rsv1;
    unsigned char LocalStatus;		/* local status register */
    unsigned char LocalAddrMod;		/* local address modifier register */
    unsigned char rsv3;
    unsigned char rsv4;
    unsigned char LocalIntrVec;		/* local interrupt vector register */
    unsigned char RemoteCmd2;		/* remote command register #2 */
    unsigned char RemoteCmd1;		/* remote command register #1 */
    unsigned char RemotePageAddrHigh;	/* high 8 bits of remote page addr */
    unsigned char RemotePageAddrLow;	/* low 8 bits of remote page addr */
    unsigned char RemoteAddrMod;	/* remote address modifier */
    unsigned char rsv5;
    unsigned char RemoteIackReadHigh;	/* interrupt acknowledge read hi */
    unsigned char RemoteIackReadLow;	/* interrupt acknowledge read low */
} CtrlRegs;

/*
 *	This is the info stored for each VME link board.
 */
typedef struct VMELinkInfo {
    volatile CtrlRegs *regArea;
    Address remoteVMEvirtual[VMELINK_NUM_PAGES];
    unsigned int addrMsb;	/* MSbit of VME addresses, since offsets */
    				/* into a file can only be 31 bits long */
    unsigned char LocalFlags;	/* flags used to set the local and remote */
    unsigned char RemoteFlags1;	/* command registers.  They are kept here */
    unsigned char RemoteFlags2;	/* so the driver can fool with the link. */
} VMELinkInfo;

static VMELinkInfo VMEInfo[8];
static int numVMElinks = 0;

#ifdef VMELINK_DEBUG
static int debug = TRUE;
#else
static int debug = FALSE;
#endif
    

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
    
    isPresent = Mach_Probe(sizeof(regsPtr->LocalStatus), (char *)&status,
			   (char *)&(regsPtr->LocalStatus));
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
 * DevVMElinkInit
 *
 *	Initialize the VME link board (assuming that there is one
 *	installed).
 *	
 *
 * Results:
 *	
 *
 * Side effects:
 *	May map additional VME address space into the kernel.
 *
 *----------------------------------------------------------------------
 */

ClientData
DevVMElinkInit(cntrlPtr)
    DevConfigController *cntrlPtr;
{
    unsigned int curStatus;
    volatile CtrlRegs *regPtr = (volatile CtrlRegs *)cntrlPtr->address;
    int i;
    register VMELinkInfo *linkInfo;

    /*
     * If the VME link board isn't installed or there are too many boards,
     * just return.
     */
    if (numVMElinks >= VMELINK_MAX_BOARDS ||
	ProbeVMEBoard (regPtr) == FALSE) {
	return (DEV_NO_CONTROLLER);
    }
    linkInfo = &(VMEInfo[numVMElinks]);

    /*
     *	Reset the remote card if it's powered on.
     */
    curStatus = (unsigned int)regPtr->LocalStatus;
    if (!(curStatus & VMELINK_REMOTE_DOWN)) {
	printf ("Resetting remote VME link card...\n");
	regPtr->RemoteCmd1 = 0x80;
	MACH_DELAY(1000000);
	regPtr->RemoteCmd1 = 0x00;
    } else {
	printf ("Remote VME link card down; can't reset.\n");
    }

#if 0
    /*
     * Attempt to map in part of the address space which corresponds
     * to the VME memory that exists on the remote system.
     */
    if (debug) {
	printf ("VMElink: mapping in 0x%x pages.\n", VMELINK_NUM_PAGES);
    }
    for (i = 0; i < VMELINK_NUM_PAGES; i++) {
	linkInfo->remoteVMEvirtual[i] =
	    (Address)VmMach_MapInDevice ((Address)(VMELINK_VME_START_ADDR +
					 i * PAGSIZ), VMELINK_ADDR_SPACE);
	if (debug) {
	    printf ("VMElink: Mapped 0x%x to 0x%x\n",
		    VMELINK_VME_START_ADDR + i * PAGSIZ,
		    linkInfo->remoteVMEvirtual[i]);
	}
    }
#endif

    linkInfo->LocalFlags = 0;
    linkInfo->RemoteFlags1 = 0;
    linkInfo->RemoteFlags2 = 0;
    linkInfo->regArea = regPtr;
    linkInfo->addrMsb = 0x80000000;
    numVMElinks += 1;
    return ((ClientData)(numVMElinks - 1));
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
    if (devicePtr->unit >= numVMElinks) {
	return (DEV_INVALID_UNIT);
    }

    devicePtr->data = (ClientData)devicePtr->unit;
    if (debug) {
	printf ("VMElink: Opened device successfully (unit %d).\n",
		devicePtr->unit);
    }

    return (SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * shortBcopy
 *
 *	Copy data from one location to another using only short (16 bit)
 *	accesses.
 *
 * Results:
 *	none
 *
 * Side effects:
 *	Data is copied.
 *
 *----------------------------------------------------------------------
 */
static
void
shortBcopy (from, to, num)
    char *from;
    char *to;
    unsigned int num;
{
    if (debug) {
	printf ("VMElink: Copying from 0x%x bytes from 0x%x to 0x%x\n",
		num, from, to);
    }
    /*
     * If pointers aren't aligned, byte by byte is the best we can do.
     */
    if (num == 0) {
	return;
    }

    if (((unsigned int)from & 1) != ((unsigned int)to & 1)) {
	while (num-- > 0) {
	    *(to++) = *(from++);
	}
    } else {
	if (((unsigned int)from & 1) == 1) {
	    *(to++) = *(from++);
	    num--;
	}
	while (num > 1) {
	    *(((short *)to)++) = *(((short *)from)++);
	    num -= 2;
	}
	if (num > 0) {
	    *to = *from;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * xferDataViaVME
 *
 *	Transfer data over the VME link using the statically set up
 *	pages.  Later on, this should be modified to use the page
 *	register (and possibly the address modifier register).
 *	
 * Results:
 *	Standard Sprite ReturnStatus.
 *
 * Side effects:
 *	Data is transferred over the VME link boards.
 *
 *----------------------------------------------------------------------
 */
static
ReturnStatus
xferDataViaVME (linkData, bufSize, bufLoc, bufAddr, direction)
    register VMELinkInfo *linkData;
    int bufSize;
    unsigned int bufLoc;
    char *bufAddr;
    int direction;
{
    char *xferPage;
    int xferOffsetInPage;
    int xferSize;
    unsigned int offsetInVME;
    volatile CtrlRegs *regPtr = linkData->regArea;

    /*
     * Correct for inability of Sprite to seek beyond 2^31 bits
     */
    bufLoc |= linkData->addrMsb;
    if ((bufLoc < VMELINK_VME_START_ADDR) ||
	(bufLoc > (VMELINK_VME_ADDR_SIZE + VMELINK_VME_START_ADDR))) {
	return (GEN_INVALID_ARG);
    }
    offsetInVME = (bufLoc - VMELINK_VME_START_ADDR);
    if (debug) {
	printf ("VMElink:  Trying I/O at 0x%x for %x bytes.\n",
		bufLoc, bufSize);
    }

    /*
     * Set up the VME link correctly.  The settings will be restored
     * later.
     */
    regPtr->LocalCmd = 0x00;
    regPtr->RemoteCmd1 = 0x00;
    regPtr->RemoteCmd2 = 0x00;

    while (bufSize > 0) {
	xferPage = linkData->remoteVMEvirtual[offsetInVME / PAGSIZ];
	xferOffsetInPage = offsetInVME % PAGSIZ;
	xferSize = ((bufSize > PAGSIZ) ? PAGSIZ : bufSize);
	xferSize = ((xferSize > (PAGSIZ - xferOffsetInPage)) ?
		    (PAGSIZ - xferOffsetInPage) : xferSize);
	if (direction == XFER_TO_REMOTE) {
	    shortBcopy (bufAddr, xferPage + xferOffsetInPage, xferSize);
	} else {
	    shortBcopy (xferPage + xferOffsetInPage, bufAddr, xferSize);
	}	    
	bufSize -= xferSize;
	offsetInVME += xferSize;
	bufAddr += xferSize;
    }

    regPtr->LocalCmd = linkData->LocalFlags;
    regPtr->RemoteCmd1 = linkData->RemoteFlags1;
    regPtr->RemoteCmd2 = linkData->RemoteFlags2;

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
    ReturnStatus retval;

    retval = xferDataViaVME (&VMEInfo[(int)devicePtr->data], readPtr->length,
			     readPtr->offset, readPtr->buffer,
			     XFER_FROM_REMOTE);
    replyPtr->flags = FS_READABLE | FS_WRITABLE;
    if (retval == SUCCESS) {
	replyPtr->length = readPtr->length;
    } else {
	replyPtr->length = 0;
    }
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
    ReturnStatus retval;

    retval = xferDataViaVME (&VMEInfo[(int)devicePtr->data], writePtr->length,
			     writePtr->offset, writePtr->buffer,
			     XFER_TO_REMOTE);
    replyPtr->flags = FS_READABLE | FS_WRITABLE;
    if (retval == SUCCESS) {
	replyPtr->length = writePtr->length;
    } else {
	replyPtr->length = 0;
    }
    return (retval);
}

/*
 *----------------------------------------------------------------------
 *
 * mapInMemory
 *
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

static
ReturnStatus
mapInMemory(inReq, outReq, linkData)
    register VMElinkMapRequest *inReq;
    register VMElinkMapRequest *outReq;
    register VMELinkInfo *linkData;
{
    return (GEN_NOT_IMPLEMENTED);
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

/* ARGSUSED */
ENTRY ReturnStatus
DevVMElinkIOControl (devicePtr, ioctlPtr, replyPtr)
    Fs_Device *devicePtr;		/* Information about device. */
    register Fs_IOCParam *ioctlPtr;	/* Parameter information (buffer sizes
					 * etc.). */
    register Fs_IOReply *replyPtr;	/* Place to store result information.*/
{
    register volatile CtrlRegs *regPtr;
    register VMELinkInfo *linkData;
    unsigned int passedData;
    ReturnStatus probeResult;
    VMElinkStatus *boardStatus;

    if (debug) {
	printf ("VMElink: doing IOControl 0x%x\n", ioctlPtr->command);
    }

    if ((int)devicePtr->data >= numVMElinks) {
	return (DEV_INVALID_UNIT);
    }
    linkData = &VMEInfo[(int)devicePtr->data];
    regPtr = linkData->regArea;

    /*
     * If the remote board isn't working, don't even bother with the
     * IOcontrol call and just return failure.
     */
    if (regPtr->RemoteCmd1 & VMELINK_REMOTE_DOWN) {
	return (DEV_OFFLINE);
    }

    switch (ioctlPtr->command) {
      case IOC_VMELINK_STATUS:
	boardStatus = (VMElinkStatus *)ioctlPtr->inBuffer;
	boardStatus->LocalStatus = (int)regPtr->LocalStatus;
	boardStatus->RemoteStatus = (int)regPtr->RemoteCmd1;
	break;
      case IOC_VMELINK_SET_ADDRMOD:
	passedData = *(unsigned int *)ioctlPtr->inBuffer;
	regPtr->RemoteAddrMod = (unsigned char)passedData;
	linkData->RemoteFlags2 |= VMELINK_REMOTE_USE_ADDRMOD;
	regPtr->RemoteCmd2 = linkData->RemoteFlags2;
	break;
      case IOC_VMELINK_NO_ADDRMOD:
	linkData->RemoteFlags2 &= ~VMELINK_REMOTE_USE_ADDRMOD;
	regPtr->RemoteCmd2 = linkData->RemoteFlags2;
	break;
      case IOC_VMELINK_NO_WINDOW:
	linkData->RemoteFlags1 &= ~VMELINK_USE_PAGE_REG;
	regPtr->RemoteCmd1 = linkData->RemoteFlags1;
	break;
      case IOC_VMELINK_SET_WINDOW:
	passedData = *(unsigned int *)ioctlPtr->inBuffer;
	linkData->RemoteFlags1 |= VMELINK_USE_PAGE_REG;
	regPtr->RemotePageAddrLow =
	    (unsigned char)((passedData & 0x00ff0000) >> 16);
	regPtr->RemotePageAddrHigh =
	    (unsigned char)((passedData & 0xff000000) >> 24);
	regPtr->RemoteCmd1 = linkData->RemoteFlags1;
	break;
      case IOC_VMELINK_SET_WINDOW_SIZE:
	passedData = *(unsigned int *)ioctlPtr->inBuffer;
	linkData->RemoteFlags2 &= ~VMELINK_WINDOW_SIZE_1M;
	linkData->RemoteFlags2 |= (unsigned char) passedData;
	regPtr->RemoteCmd2 = linkData->RemoteFlags2;
	break;
      case IOC_VMELINK_MAP_MEMORY:
	probeResult = mapInMemory ((VMElinkMapRequest *)ioctlPtr->inBuffer,
				   (VMElinkMapRequest *)ioctlPtr->outBuffer,
				   linkData);
	if (probeResult != SUCCESS) {
	    replyPtr->length = ioctlPtr->outBufSize = 0;
	    return (probeResult);
	}
	break;
      case IOC_VMELINK_LOW_VME:
	linkData->addrMsb = 0x00000000;
	break;
      case IOC_VMELINK_HIGH_VME:
	linkData->addrMsb = 0x80000000;
	break;
      case IOC_VMELINK_DEBUG_ON:
	debug = TRUE;
	break;
      case IOC_VMELINK_DEBUG_OFF:
	debug = FALSE;
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
      case IOC_REPOSITION:
	break;
      case IOC_GET_FLAGS:
      case IOC_SET_FLAGS:
      case IOC_SET_BITS:
      case IOC_CLEAR_BITS:
	break;
      default:
	return (GEN_NOT_IMPLEMENTED);
	break;
    }

    return (SUCCESS);
}

