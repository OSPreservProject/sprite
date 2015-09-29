/* 
 * emuInit.c --
 *
 *	Initialization for the Sprite emulator library.
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
static char rcsid[] = "$Header: /r3/kupfer/spriteserver/src/lib/c/emulator/RCS/emuInit.c,v 1.2 91/10/04 12:03:01 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <mach.h>
#include <spriteEmu.h>
#include <spriteEmuInt.h>

/* 
 * This should go into a Mach header file somewhere...
 */
extern int mach_init();

/* 
 * This is the task's port for making Sprite requests.  Callers should use 
 * the SpriteEmu_ServerPort function to get it, to ensure that the port is 
 * correctly initialized.
 */
static mach_port_t serverPort = MACH_PORT_NULL; 
				/* port for making Sprite requests */


/*
 *----------------------------------------------------------------------
 *
 * SpriteEmu_Init --
 *
 *	Initialization for the Sprite emulation library.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Ensures that Mach initialization is done.  Gets the Sprite request
 *	port for the process.  Suspends the current thread if there was an
 *	error.
 *
 *----------------------------------------------------------------------
 */

void
SpriteEmu_Init()
{
    mach_init();

    if (task_get_bootstrap_port(mach_task_self(), &serverPort)
	== KERN_SUCCESS) {
	return;
    }

    thread_suspend(mach_thread_self());
}


/*
 *----------------------------------------------------------------------
 *
 * SpriteEmu_ServerPort --
 *
 *	Get the service request port.
 *
 * Results:
 *	Returns the service port.
 *
 * Side effects:
 *	Initializes the emulation support, if necessary.
 *
 *----------------------------------------------------------------------
 */

mach_port_t
SpriteEmu_ServerPort()
{
    if (serverPort == MACH_PORT_NULL) {
	SpriteEmu_Init();
    }

    return serverPort;
}
