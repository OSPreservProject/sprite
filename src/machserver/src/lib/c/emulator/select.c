/* 
 * select.c --
 *
 *	Procedure to map from Unix select system call to Sprite.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/lib/c/emulator/RCS/select.c,v 1.3 92/03/12 19:22:29 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <mach.h>
#include <status.h>
#include <fs.h>
#include "compatInt.h"
#include <sys/time.h>
#include <spriteEmuInt.h>


/*
 *----------------------------------------------------------------------
 *
 * select --
 *
 *	Procedure to map from Unix select system call to Sprite Fs_Select.
 *
 * Results:
 *	UNIX_ERROR is returned upon error, with the actual error code
 *	stored in errno.  The number of ready descriptors is returned
 *	upon success.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
select(width, readfds, writefds, exceptfds, timeout)
    int width, *readfds, *writefds, *exceptfds;
    struct timeval *timeout;
{
    ReturnStatus status;
    kern_return_t kernStatus;
    int numReady;
    Time spriteTimeout;
    Time *timeoutPtr = &spriteTimeout;
    Boolean useTimeout = FALSE;
    Boolean sigPending;

    if (timeout != NULL) {
	useTimeout = TRUE;
	spriteTimeout.seconds = timeout->tv_sec;
	spriteTimeout.microseconds = timeout->tv_usec;
    }
    kernStatus = Fs_SelectStub(SpriteEmu_ServerPort(), width, useTimeout,
			       timeoutPtr, (vm_address_t)readfds,
			       (vm_address_t)writefds,
			       (vm_address_t)exceptfds, &status, &numReady,
			       &sigPending);
    if (kernStatus != KERN_SUCCESS) {
	status = Utils_MapMachStatus(kernStatus);
    }
    if (sigPending) {
	SpriteEmu_TakeSignals();
    }
    if (status != SUCCESS) {
	if (status == FS_TIMEOUT) {
	    if (readfds != NULL) {
		*readfds = 0;
	    }
	    if (writefds != NULL) {
		*writefds = 0;
	    }
	    if (exceptfds != NULL) {
		*exceptfds = 0;
	    }
	    return(0);
	} else {
	    errno = Compat_MapCode(status);
	    return(UNIX_ERROR);
	}
    } else {
	return(numReady);
    }
}
