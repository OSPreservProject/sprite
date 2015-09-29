/*
 * dev.c --
 *
 *	Excerpts from the dev module in the kernel.
 * Copyright 1985 Regents of the University of California
 * All rights reserved.
 */

#ifdef notdef
static char rcsid[] = "$Header: /sprite/src/kernel/dev/sun3.md/RCS/devInit.c,v 8.3 89/05/24 07:50:56 rab Exp $ SPRITE (Berkeley)";
#endif 

#include "sprite.h"
#include "devInt.h"
#include "vmSunConst.h"




/*
 *----------------------------------------------------------------------
 *
 * Dev_Config --
 *
 *	Call the initialization routines for various controllers and
 *	devices based on configuration tables.  This should be called
 *	after the regular memory allocater is set up so the drivers
 *	can use it to configure themselves.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Call the controller and device initialization routines.
 *
 *----------------------------------------------------------------------
 */
void
Dev_Config()
{
    int memoryType;		/* This is ultimatly for the page table
				 * type field used to map in the device */
    Boolean mapItIn;		/* If TRUE we need to map the device into
				 * kernel virtual space */
    register DevConfigController *cntrlrPtr;

	cntrlrPtr = &devCntrlr[0];
	/*
	 * The following makes sure that the physical address for the
	 * controller is correct, and then maps that into the kernel's virtual
	 * address space (if it hasn't already been done by the Boot PROM).
	 * There are two things happening here.  First, each device has
	 * its own physical address on its own kind of bus - A device may
	 * think it's in Multibus Memory, or VME bus 16 bit data 16 bit address,
	 * or some other permutation.  For Multibus Memory devices, like
	 * the SCSI controller, Multibus memory is the low megabyte of
	 * the physical addresses, with Multibus I/O being the last 64K
	 * of this first Meg.  Furthermore, the Boot PROM maps this low
	 * meg of physical memory into the 16'th Meg of the kernel's virtual
	 * address space.  All that needs to be done is imitate this mapping
	 * by adding the MULTIBUS BASE to the controller address and the
	 * controller can forge ahead.
	 * It's more complicated for VME devices.  There are six subsets
	 * of address/data spaces supported by the VME bus, and each one
	 * belongs to a special range of physical addresses and has a
	 * corresponding "page type" which goes into the page table.
	 * Furthermore, we arn't depending on the boot PROM and so once
	 * the correct physical address is gen'ed up we need to map it
	 * into a kernel virtual address that the device driver can use.
	 */
	switch(cntrlrPtr->space) {
	    case DEV_VME_D16A16:
		/*
		 * The high 64K of the VME address range is stolen for the
		 * 16 bit address subset.
		 */
		mapItIn = TRUE;
		 memoryType = 2;
		cntrlrPtr->address += 0xFFFF0000;
		break;
	    case DEV_VME_D16A24:
		/*
		 * The high 16 Megabytes of the VME address range is stolen
		 * for the 24 bit address subset.
		 */
		mapItIn = TRUE;
		 memoryType = 2;
		cntrlrPtr->address += 0xFF000000;
		break;
	    case DEV_OBIO:
		mapItIn = FALSE;
		memoryType = 1;
		break;
	}
#ifdef sun2
                    if (mapItIn && cntrlrPtr->address >= 0xFF800000) {
                        memoryType = 3;
                    } else {
                        memoryType = 2;
                    }
#endif
	if (mapItIn) {
	    cntrlrPtr->address =
		(int)VmMach_MapInDevice((Address)cntrlrPtr->address,memoryType);
	}
	     (*cntrlrPtr->initProc)(cntrlrPtr);
}


