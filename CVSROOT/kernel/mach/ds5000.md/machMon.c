/* 
 * machMon.c --
 *
 *	Routines to access the PMAX prom monitor.
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
static char rcsid[] = "$Header$ SPRITE (DECWRL)";
#endif not lint

#include "sprite.h"
#define _MONFUNCS
#include "machMon.h"
#include "machConst.h"
#include "machInt.h"
#include "ctype.h"
#include "mach.h"


/*
 * ----------------------------------------------------------------------------
 *
 * Mach_MonAbort --
 *
 *     Abort to prom.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     Aborts to monitor.
 *
 * ----------------------------------------------------------------------------
 */
void
Mach_MonAbort()
{
	mach_MonFuncs.halt();
}

/*
 * ----------------------------------------------------------------------------
 *
 * Mach_MonPutChar --
 *
 *     Call the monitor put character routine
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */
int
Mach_MonPutChar(ch)
    int		ch;
{
    return(Dev_GraphicsPutc(ch));
}


/*
 * ----------------------------------------------------------------------------
 *
 * Mach_MonReboot --
 *
 *     	Reboot the system.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     System rebooted.
 *
 * ----------------------------------------------------------------------------
 */
void
Mach_MonReboot(rebootString)
    char	*rebootString;
{
    char *bootpath;
    if (*rebootString != '\0') {
	mach_MonFuncs.setenv2("bootpath",rebootString);
    } else {
	bootpath = mach_MonFuncs.getenv2("bootpath");
	if (bootpath == (char *)NULL || *bootpath == '\0') {
	    /*
	     * Hardware doesn't have a bootpath.
	     */
	    mach_MonFuncs.setenv2("bootpath",DEFAULT_REBOOT);
	    printf("Using default %s\n",DEFAULT_REBOOT);
	}
	/*
	 * Otherwise use hardware's bootpath.
	 */
    }
    *MACH_USE_NON_VOLATILE |= MACH_NON_VOLATILE_FLAG;
    mach_MonFuncs.autoboot();
    panic("Mach_MonReboot: Reboot failed (I'm still alive aren't I?)\n");
}
