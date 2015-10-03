/*
 * devInit.c --
 *
 *	This has a weak form of autoconfiguration for mapping in various
 *	devices and calling their initialization routines.  The file
 *	devConfig.c has the tables which list devices, their physical
 *	addresses, and their initialization routines.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.  
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "devInt.h"
#include "graphics.h"
#include "vm.h"
#include "dbg.h"
#include "sysStats.h"
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
    DevTtyInit();
    DevGraphicsInit();
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
    register int			index;
    ClientData				callBackData;
    register DevConfigController	*cntrlrPtr;

    if (devConfigDebug) {
	printf("Dev_Config calling debugger:");
	DBG_CALL;
    }
    for (index = 0; index < devNumConfigCntrlrs; index++) {
	cntrlrPtr = &devCntrlr[index];
	callBackData = (*cntrlrPtr->initProc)(cntrlrPtr);
	if (callBackData != DEV_NO_CONTROLLER) {
	    printf("%s at kernel address %x\n", cntrlrPtr->name,
			   cntrlrPtr->address);
	}
    }
}
