/* 
 * fcntl.c --
 *
 *	Procedure to map the Unix fcntl system call to Sprite.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/lib/c/emulator/RCS/fcntl.c,v 1.3 92/03/12 19:22:25 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <mach.h>
#include <fs.h>
#include <fcntl.h>
#include <stdio.h>

#include "compatInt.h"
#include <spriteEmuInt.h>


/*
 *----------------------------------------------------------------------
 *
 * fcntl --
 *
 *	Procedure to map from Unix fcntl system call to Sprite Fs_IOControl.
 *
 * Results:
 *	a value depending on the command, or
 *      UNIX_SUCCESS     the call was successful, or
 *      UNIX_ERROR       the call was not successful.
 *                        The actual error code stored in errno.
 *
 * Side effects:
 *	Variable.
 *
 *----------------------------------------------------------------------
 */

int
fcntl(fd, cmd, arg)
    int fd;		/* File to operate on. */
    int cmd;		/* Type of command. */
    int arg;		/* Optional argument to the command. */
{
    ReturnStatus status;
    kern_return_t kernStatus;
    int value;
    int temp;
    Ioc_Owner owner;
    Boolean sigPending = FALSE;

    switch (cmd) {
    case F_DUPFD:
	kernStatus = Fs_GetNewIDStub(SpriteEmu_ServerPort(), fd, &arg,
				     &status, &sigPending);
	if (kernStatus != KERN_SUCCESS) {
	    status = Utils_MapMachStatus(kernStatus);
	}
	value = arg;
	break;

    case F_GETFD:
	status = Fs_IOControl(fd, IOC_GET_FLAGS, 
			      0, (Address) NULL, 
			      sizeof(value), (Address) &value);
	value = (value & IOC_CLOSE_ON_EXEC) ? 1 : 0;
	break;

    case F_SETFD:
	value = IOC_CLOSE_ON_EXEC;
	if (arg & 1) {
	    status = Fs_IOControl(fd, IOC_SET_BITS, 
				  sizeof(value), (Address) &value,
				  0, (Address) NULL);
	} else {
	    status = Fs_IOControl(fd, IOC_CLEAR_BITS, 
				  sizeof(value), (Address) &value,
				  0, (Address) NULL);
	}
	value = UNIX_SUCCESS;
	break;

    case F_GETFL:
	status = Fs_IOControl(fd, IOC_GET_FLAGS, 
			      0, (Address) NULL, 
			      sizeof(temp), (Address) &temp);
	value = 0;
	if (temp & IOC_APPEND) {
	    value |= FAPPEND;
	}
	if (temp & IOC_NON_BLOCKING) {
	    value |= FNDELAY;
	}
	if (temp & IOC_ASYNCHRONOUS) {
	    value |= FASYNC;
	}
	switch(temp & (IOC_READ | IOC_WRITE)) {
	case IOC_READ :
	    value |= O_RDONLY;
	    break;
	case IOC_WRITE:
	    value |= O_WRONLY;
	    break;
	case (IOC_READ | IOC_WRITE) :
	    value |= O_RDWR;
	    break;
	}
	break;
	
    case F_SETFL:
	value = 0;
	if (arg & FAPPEND) {
	    value |= IOC_APPEND;
	}
	if (arg & FNDELAY) {
	    value |= IOC_NON_BLOCKING;
	}
	if (arg & FASYNC) {
	    value |= IOC_ASYNCHRONOUS;
	}
	status = Fs_IOControl(fd, IOC_SET_FLAGS, 
			      sizeof(value), (Address) &value,
			      0, (Address) NULL);
	value = UNIX_SUCCESS;
	break;
	
    case F_GETOWN:
	status = Fs_IOControl(fd, IOC_GET_OWNER, 
			      0, (Address) NULL,
			      sizeof(owner), (Address) &owner);
	if (owner.procOrFamily == IOC_OWNER_FAMILY) {
	    value = -owner.id;
	} else {
	    value = owner.id;
	}
	break;
	
    case F_SETOWN:
	if (arg < 0) {
	    owner.id = -arg;
	    owner.procOrFamily = IOC_OWNER_FAMILY;
	} else {
	    owner.id = arg;
	    owner.procOrFamily = IOC_OWNER_PROC;
	}
	status = Fs_IOControl(fd, IOC_SET_OWNER, 
			      sizeof(owner), (Address) &owner,
			      0, (Address) NULL);
	value = UNIX_SUCCESS;
	break;
	
    default:
	break;
    }

    if (sigPending) {
	SpriteEmu_TakeSignals();
    }
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } 
    return(value);
}
