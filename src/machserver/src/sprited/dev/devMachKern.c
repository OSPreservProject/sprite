/* 
 * devMachKern.c --
 *
 *	Code to deal with the Mach kernel's device interface.
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
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/sprited/dev/RCS/devMachKern.c,v 1.2 92/03/23 14:38:48 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <mach.h>
#include <stdlib.h>
#include <string.h>

#include <dev.h>
#include <devInt.h>
#include <sys.h>
#include <utils.h>

/* 
 * This is the port for making device requests.
 */
mach_port_t dev_ServerPort;


/*
 *----------------------------------------------------------------------
 *
 * Dev_Init --
 *
 *	Initialization for the dev module.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Initializes the device request port and the console device.
 *
 *----------------------------------------------------------------------
 */

void
Dev_Init()
{
    int retCode;		/* UNIX errno code */

    /* 
     * The startup server has to roll its own RPC request to get the device
     * control port from the kernel (see the UX code for an example).
     * However, it is the only server that can do that, because the kernel
     * only answers once.
     */
    retCode = Utils_UnixPidToTask(UTILS_DEVICE_PID, &dev_ServerPort);
    if (retCode != 0) {
	printf("Dev_Init: can't get device server port: %s\n",
	       strerror(retCode));
	exit(1);
    }

    if (DevTtyAttach(DEV_CONSOLE_UNIT) == NULL) {
	printf("Dev_Init: can't open console.\n");
	exit(1);
    }
}
