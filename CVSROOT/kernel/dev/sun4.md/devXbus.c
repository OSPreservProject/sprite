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
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
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

static int		devXbusDebug = TRUE;
static int		devXbusModuleInitted = FALSE;
static DevXbusInfo	*xbusInfo[DEV_XBUS_MAX_BOARDS];
static int		devXbusNumBoards = 0;

/*----------------------------------------------------------------------
 *
 * DevXbusResetBoard
 *
 *	Reset the xbus board by poking the reset registers.  Get the
 *	mutex semaphore first, though.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
DevXbusResetBoard (infoPtr)
DevXbusInfo*	infoPtr;
{
    ReturnStatus status = SUCCESS;
    DevXbusCtrlRegs *regPtr = infoPtr->regs;
    unsigned int resetValue;

    MASTER_LOCK (&infoPtr->mutex);
    resetValue = DEV_XBUS_RESETREG_RESET;
    status = Mach_Probe (sizeof (regPtr->reset),(Address)&(regPtr->reset),
			 (Address)&resetValue);
    if (status != SUCCESS) {
	goto resetDone;
    }
    MACH_DELAY (500000);
    resetValue = DEV_XBUS_RESETREG_NORMAL;
    status = Mach_Probe (sizeof (regPtr->reset),(Address)&(regPtr->reset),
			 (Address)&resetValue);

  resetDone:
    MASTER_UNLOCK (&infoPtr->mutex);
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
ClientData
DevXbusInit (ctrlPtr)
DevConfigController *ctrlPtr;
{
    ReturnStatus	status = SUCCESS;
    DevXbusCtrlRegs*	regPtr = (DevXbusCtrlRegs *)ctrlPtr->address;
    DevXbusInfo*	infoPtr = NULL;
    char semName[40];

    if (!devXbusModuleInitted) {
	int i;
	for (i = 0; i < DEV_XBUS_MAX_BOARDS; i++) {
	    xbusInfo[i] = NULL;
	}
	devXbusModuleInitted = TRUE;
    }

    if (devXbusNumBoards >= DEV_XBUS_MAX_BOARDS) {
	status = DEV_NO_DEVICE;
	goto initExit;
    }

    if ((infoPtr = (DevXbusInfo *) malloc (sizeof (DevXbusInfo))) == NULL) {
	printf ("DevXbusInit: Couldn't allocate space for board info.\n");
	status = FAILURE;
	goto initExit;
    }

    xbusInfo[devXbusNumBoards++] = infoPtr;
    infoPtr->regs = regPtr;
    infoPtr->state = 0;
    infoPtr->name = ctrlPtr->name;
#if 0
    if ((infoPtr->hippisCtrlFifo = (vuint *)VmMach_MapInDevice
	 ((Address)(DEV_XBUS_REGISTER_BASE + DEV_XBUS_REG_HIPPIS_CTRL_FIFO),
	  DEV_XBUS_ADDR_SPACE)) == NULL) {
	status = FAILURE;
	goto initExit;
    }
    if ((infoPtr->hippidCtrlFifo = (vuint *)VmMach_MapInDevice
	 ((Address)(DEV_XBUS_REGISTER_BASE + DEV_XBUS_REG_HIPPID_CTRL_FIFO),
	  DEV_XBUS_ADDR_SPACE)) == NULL) {
	status = FAILURE;
	goto initExit;
    }
    if ((infoPtr->xorCtrlFifo = (vuint *)VmMach_MapInDevice
	 ((Address)(DEV_XBUS_REGISTER_BASE + DEV_XBUS_REG_XOR_CTRL_FIFO),
	  DEV_XBUS_ADDR_SPACE)) == NULL) {
	status = FAILURE;
	goto initExit;
    }
    sprintf (semName, "XbusMutex 0x%x", devXbusNumBoards-1);
    Sync_SemInitDynamic (&infoPtr->mutex, semName);
    status = DevXbusResetBoard (infoPtr);
    if (status != SUCCESS) {
	infoPtr->state |= DEV_XBUS_STATE_OK;
    } else {
	status = DEV_NO_DEVICE;
    }
#endif

  initExit:
    if (status != SUCCESS) {
	printf ("Didn't find xbus device at 0x%x\n", DEV_XBUS_REGISTER_BASE);
    } else {
	printf ("Found %s (xbus%d) at 0x%x\n", ctrlPtr->name,
		devXbusNumBoards-1, DEV_XBUS_REGISTER_BASE);
    }

    return ((status == SUCCESS) ? (ClientData)infoPtr : DEV_NO_CONTROLLER);
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
accessXbusRegister (infoPtr, regNum, value, accessType)
DevXbusInfo	*infoPtr;
int		regNum;
unsigned int*	value;
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
	if ((regNum > 0) && (regNum <= DEV_XBUS_REG_STATUS)) {
	    regAddr = (Address)infoPtr->regs + regNum;
	} else {
	    status = GEN_INVALID_ARG;
	    goto endAccess;
	}
    }

    if (accessType == IOC_XBUS_READ_REG) {
	status = Mach_Probe (sizeof (*value), (Address)value, regAddr);
    } else if (accessType == IOC_XBUS_WRITE_REG) {
	status = Mach_Probe (sizeof (*value), regAddr, (Address)value);
    } else {
	status = GEN_INVALID_ARG;
    }
  endAccess:
    return (status);
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
    ReturnStatus status = SUCCESS;
    ReturnStatus fmtStatus;

    if (devXbusDebug) {
	printf ("DevXbusIOControl: doing IOControl 0x%x\n", ioctlPtr->command);
    }

    if ((int)devicePtr->data >= devXbusNumBoards) {
	return (DEV_INVALID_UNIT);
    }
    infoPtr = xbusInfo[(int)devicePtr->data];
    if (infoPtr == NULL) {
	return (DEV_INVALID_UNIT);
    }
    regPtr = infoPtr->regs;

    switch (ioctlPtr->command) {
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
      case IOC_XBUS_RESET:
	status = DevXbusResetBoard (infoPtr);
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
Boolean
DevXbusIntr (data)
ClientData	data;
{
    DevXbusInfo*	infoPtr = (DevXbusInfo*)data;

    if (devXbusDebug) {
	printf ("DevXbusIntr: got interrupt from %s.\n", infoPtr->name);
    }
    return (TRUE);
}
