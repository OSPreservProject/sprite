/*
 * devInit.c --
 *
 *	This has a weak form of autoconfiguration for mapping in various
 *	devices and calling their initialization routines.  The file
 *	devConfig.c has the tables which list devices, their physical
 *	addresses, and their initialization routines.
 *
 *	The Sun Memory Managment setup is explained in the proprietary
 *	architecture documents, and also in the manual entitled
 *	"Writing Device Drivers for the Sun Workstation"
 *
 *
 * Copyright 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "devInt.h"
#include "devKeyboard.h"
#include "devMultibus.h"
#include "vm.h"
#include "vmMach.h"
#include "dbg.h"
#include "string.h"


int devConfigDebug = FALSE;

/*
 * ----------------------------------------------------------------------------
 *
 * Dev_Init --
 *
 *	Initialize the timer and the keyboard, and set up the allocator
 *	for the device spaces.  Device initialization routines are called
 *	later by Dev_Config.
 *
 * Results:
 *     none
 *
 * Side effects:
 *     Some devices are initialized, and the IO buffer allocater is set up.
 *
 * ----------------------------------------------------------------------------
 */

void
Dev_Init()
{
    Dev_KbdInit();
}


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
    register int index;
    int memoryType;		/* This is ultimatly for the page table
				 * type field used to map in the device */
    Boolean mapItIn;		/* If TRUE we need to map the device into
				 * kernel virtual space */

    if (devConfigDebug) {
	printf("Dev_Config calling debugger:");
	DBG_CALL;
    }
    for (index = 0 ; index < devNumConfigCntrlrs ; index++) {
	register DevConfigController *cntrlrPtr;
	cntrlrPtr = &devCntrlr[index];
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
	if (Mach_GetMachineType() == SYS_SUN_2_120 &&
	    cntrlrPtr->space != DEV_MULTIBUS &&
	    cntrlrPtr->space != DEV_MULTIBUS_IO) {
	    /*
	     * Mapping VME addresses into a multibus based kernel
	     * messes things up.
	     */
	    continue;
	}
	switch(cntrlrPtr->space) {
	    case DEV_MULTIBUS:
	    case DEV_MULTIBUS_IO:
		/*
		 * Things are already mapped in by the Boot PROM.
		 * We just relocate the address into the high meg.
		 */
		if (Mach_GetMachineType() != SYS_SUN_2_120) {
		    continue;
		}
		mapItIn = FALSE;
		cntrlrPtr->address += DEV_MULTIBUS_BASE;
		break;
	    /*
	     * We have to relocate the controller address to the
	     * true VME PHYSICAL address that the Sun MMU uses for
	     * the different types of VME spaces.
	     */
	    case DEV_VME_D32A16:
	    case DEV_VME_D16A16:
		/*
		 * The high 64K of the VME address range is stolen for the
		 * 16 bit address subset.
		 */
		mapItIn = TRUE;
		cntrlrPtr->address += 0xFFFF0000;
		break;
	    case DEV_VME_D32A24:
	    case DEV_VME_D16A24:
		/*
		 * The high 16 Megabytes of the VME address range is stolen
		 * for the 24 bit address subset.
		 */
		mapItIn = TRUE;
		cntrlrPtr->address += 0xFF000000;
		break;
		/*
		 * The addresses for the full 32 bit VME bus are ok.
		 */
	    case DEV_VME_D16A32:
	    case DEV_VME_D32A32:
		mapItIn = TRUE;
		break;
	    case DEV_OBIO:
		mapItIn = FALSE;
		break;
	}
	/*
	 * Each different Sun architecture arranges pieces of memory into
	 * 4 different spaces.  The I/O devices fall into type 2 or 3 space.
	 */
	switch(cntrlrPtr->space) {
	    case DEV_MULTIBUS:
		memoryType = 2;
		break;
	    case DEV_VME_D16A16:
	    case DEV_VME_D16A24:
	    case DEV_VME_D16A32:
		if (Mach_GetMachineType() == SYS_SUN_2_50) {
		    if (cntrlrPtr->address >= 0xFF800000) {
			memoryType = 3;
		    } else {
			memoryType = 2;
		    }
		} else {
		    memoryType = 2;
		}
		break;
	    case DEV_MULTIBUS_IO:
	    case DEV_VME_D32A16:
	    case DEV_VME_D32A24:
	    case DEV_VME_D32A32:
		memoryType = 3;
		break;
	    case DEV_OBIO:
		memoryType = 1;
		break;
	}
	if (mapItIn) {
	    cntrlrPtr->address =
		(int)VmMach_MapInDevice((Address)cntrlrPtr->address,memoryType);
	}
	if (cntrlrPtr->address != NIL) {
	    ClientData	callBackData;
	    callBackData = (*cntrlrPtr->initProc)(cntrlrPtr);
	    if (callBackData != DEV_NO_CONTROLLER) {
		printf("%s at kernel address %x\n", cntrlrPtr->name,
			       cntrlrPtr->address);
		if (cntrlrPtr->vectorNumber > 0) {
		    Mach_SetHandler(cntrlrPtr->vectorNumber,
			cntrlrPtr->intrProc, callBackData);
		}
	    }
	}
    }
}

