/* 
 * devStdFB.c --
 *
 *	Routines for the /dev/stdfb device.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <devGraphicsInt.h>
#include <proc.h>
#include <dev/stdfb.h>
#include <devStdFBInt.h>

/*
 * Info on each process that has the device open.
 */
typedef struct OpenInfo {
    List_Links			links;		/* Link them together. */
    Proc_ControlBlock		*procPtr;	/* Process with fb open. */
    Address			addr;		/* Address where fb is 
						 * mapped. */
} OpenInfo;

static List_Links	infoList;


/*
 *----------------------------------------------------------------------
 *
 *  DevStdFBOpen--
 *
 *	Open the device.
 *
 * Results:
 *	SUCCESS if the device was opened, FAILURE otherwise
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
DevStdFBOpen(devicePtr, useFlags, token, flagsPtr)
    Fs_Device	*devicePtr;	/* Device info, unit number, etc. */
    int		useFlags;	/* Flags from the stream being opened. */
    Fs_NotifyToken	token;	/* Call-back token for input, unused here. */
    int		*flagsPtr;	/* OUT: Device open flags. */
{
    int			slot;
    Mach_SlotInfo	slotInfo;
    char		*slotAddr;
    OpenInfo		*infoPtr;
    Proc_ControlBlock	*procPtr;
    ReturnStatus	status = SUCCESS;

    procPtr = Proc_GetCurrentProc();
    if (devicePtr->data == (ClientData) NIL) {
	List_Init(&infoList);
	devicePtr->data = (ClientData) &infoList;
    }
    slot = devicePtr->unit;
    slotAddr = (char *) MACH_IO_SLOT_ADDR(slot);
    status = Mach_GetSlotInfo(slotAddr + PMAGBA_ROM_OFFSET, &slotInfo);
    if (status != SUCCESS) {
	return status;
    }
    if (strcmp(slotInfo.vendor, "DEC") || 
	strcmp(slotInfo.module, "PMAG-BA")) {
	return FAILURE;
    }
    infoPtr = (OpenInfo *) malloc(sizeof(OpenInfo));
    bzero((char *) infoPtr, sizeof(OpenInfo));
    List_InitElement((List_Links *) infoPtr);
    infoPtr->procPtr = procPtr;
    infoPtr->addr = (Address) NIL;
    List_Insert((List_Links *) infoPtr, LIST_ATFRONT(&infoList));
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * DevStdFBMMap --
 *
 *	Map in the frame buffer.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
DevStdFBMMap(devicePtr, startAddr, length, offset, newAddrPtr)
    Fs_Device	*devicePtr;	/* Device to map. */
    Address	startAddr;	/* Where to map it.
    int		length;		/* Number of bytes to map. */
    int		offset;		/* Unused. */
    Address	*newAddrPtr;	/* New address. */
{
    ReturnStatus	status = SUCCESS;
    char		*fbAddr;
    OpenInfo		*infoPtr = (OpenInfo *) NIL;
    Proc_ControlBlock	*procPtr;
    int			addr;
    Boolean		found = FALSE;

    procPtr = Proc_GetCurrentProc();
    LIST_FORALL((List_Links *) devicePtr->data, (List_Links *) infoPtr) {
	if (infoPtr->procPtr == procPtr) {
	    if (infoPtr->addr != (Address) NIL) {
		return FAILURE;
	    } else {
		found = TRUE;
		break;
	    }
	}
    }
    if (!found) {
	panic("DevStdFBMMap: info for proc 0x%x not found\n", procPtr);
	return FAILURE;
    }
    fbAddr = (char *) MACH_IO_SLOT_ADDR(devicePtr->unit) + PMAGBA_BUFFER_OFFSET;
    addr = (int) startAddr;
    if (addr & VMMACH_OFFSET_MASK) {
	addr += VMMACH_PAGE_SIZE;
	addr &= ~VMMACH_OFFSET_MASK;
    }
    status = VmMach_UserMap(length, (Address) addr, (Address) fbAddr, FALSE,
		(Address *) &addr);
    if (status != SUCCESS) {
	return status;
    }
    infoPtr->addr = (Address) addr;
    *newAddrPtr = (Address) addr; 
}

/*
 *----------------------------------------------------------------------
 *
 *  DevStdFBIOControl --
 *
 *	Perform an IO control.
 *
 * Results:
 *      GEN_NOT_IMPLEMENTED if io control not supported.  GEN_INVALID_ARG
 *	if something else went wrong.  SUCCESS otherwise.
 *	
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
DevStdFBIOControl(devicePtr, ioctlPtr, replyPtr)
    Fs_Device	*devicePtr;	/* Handle for device. */
    Fs_IOCParam	*ioctlPtr;	/* Standard I/O Control parameter block. */
    Fs_IOReply	*replyPtr;	/* Size of outBuffer and returned signal. */
{
    Dev_StdFBInfo	info;

    switch(ioctlPtr->command) {
	case IOC_STDFB_INFO: {
	    if (ioctlPtr->outBufSize < sizeof (Dev_StdFBInfo)) {
		return GEN_INVALID_ARG;
	    }
	    info.type = PMAGBA;
	    info.height = PMAGBA_HEIGHT;
	    info.width = PMAGBA_WIDTH;
	    info.planes = PMAGBA_PLANES;
	    bcopy((char *) &info, (char *) ioctlPtr->outBuffer, sizeof(info));
	    break;
	}
	default: 
	    return GEN_NOT_IMPLEMENTED;
    }
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * DevStdFBClose --
 *
 *	Close the device
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
DevStdFBClose(devicePtr, useFlags, openCount, writerCount)
    Fs_Device   *devicePtr;		/* Information about device. */
    int         useFlags;		/* Indicates whether stream being
					 * closed was for reading and/or
					 * writing:  OR'ed combination of
					 * FS_READ and FS_WRITE. */
    int         openCount;		/* # of times this particular stream
					 * is still open. */
    int         writerCount;		/* # of times this particular stream
					 * is still open for writing. */
{
    ReturnStatus	status = SUCCESS;
    OpenInfo		*infoPtr = (OpenInfo *) NIL;
    Proc_ControlBlock	*procPtr;

    procPtr = Proc_GetCurrentProc();
    LIST_FORALL((List_Links *) devicePtr->data, (List_Links *) infoPtr) {
	if (infoPtr->procPtr == procPtr) {
	    break;
	}
    }
    if (infoPtr->addr != (Address) NIL) {
	status = VmMach_UserUnmap(infoPtr->addr);
	if (status != SUCCESS) {
	    printf("DevFBClose: couldn't unmap frame buffer\n");
	    return FAILURE;
	}
    }
    List_Remove((List_Links *) infoPtr);
    free((char *) infoPtr);
    if (openCount == 0) {
	devicePtr->data = (ClientData) NIL;
    }
    return status;
}

