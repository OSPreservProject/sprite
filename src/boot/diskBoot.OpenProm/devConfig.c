/*
 * devConfig.c --
 *
 *	Excerpts from the dev module in the kernel.
 *
 * Copyright 1985 Regents of the University of California
 * All rights reserved.
 */

#ifdef notdef
static char rcsid[] = "$Header: /sprite/src/boot/diskBoot.OpenProm/RCS/devConfig.c,v 1.1 90/09/17 11:01:50 rab Exp $ SPRITE (Berkeley)";
#endif 

#include "sprite.h"
#include "fsBoot.h"
#include "vmSunConst.h"
#include "machMon.h"




/*
 *----------------------------------------------------------------------
 *
 * dev_config --
 *
 *	Boottime device configuration.  This is a special version of this
 *	routine that knows we will be using the PROM driver routines.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	This saves the pointer to the boot parameters in the global
 *	fsDevice so that it is available to the SunPromDevOpen and
 *	SunPromDevRead routines.
 *
 *----------------------------------------------------------------------
 */
void
dev_config(devName, fsDevicePtr)
    char *devName;		/* from PROM parameters */
    Fs_Device *fsDevicePtr;	/* FS descriptor for the boot device */
{
    fsDevicePtr->serverID = -1;
    fsDevicePtr->type = 0;
    fsDevicePtr->data = (ClientData)devName;
}
