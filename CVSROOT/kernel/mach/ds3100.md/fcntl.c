/* 
 * fcntl.c --
 *
 *	Procedure to map the Unix fcntl system call to Sprite.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "fs.h"

#include "compatInt.h"

#include "user/fcntl.h"
#include "machInt.h"
#include "machConst.h"
#include "mach.h"

extern Mach_State	*machCurStatePtr;


/*
 *----------------------------------------------------------------------
 *
 * fcntl --
 *
 *	Procedure to map from Unix fcntl system call to Sprite Fs_IOControl.
 *
 * Results:
 *      Error returned if error, SUCCESS otherwise.
 *
 * Side effects:
 *	Variable.
 *
 *----------------------------------------------------------------------
 */

int
MachUNIXFcntl(retValPtr, fd, cmd, arg)
    int *retValPtr;
    int fd;		/* File to operate on. */
    int cmd;		/* Type of command. */
    int arg;		/* Optional argument to the command. */
{
    ReturnStatus status;
    int value;
    Address		usp;

    usp = (Address)machCurStatePtr->userState.regState.regs[SP];

    switch (cmd) {
	case F_DUPFD:
	    usp -= sizeof(int);
	    status = Vm_CopyOut(sizeof(int), (Address)&arg, usp);
	    if (status != SUCCESS) {
		return(status);
	    }
	    status = Fs_GetNewIDStub(fd, (int *)usp);
	    if (status == SUCCESS) {
		(void)Vm_CopyIn(sizeof(int), usp, (Address)retValPtr);
	    }
	    break;

	case F_GETFD:
	    usp -= sizeof(int);
	    status = Fs_IOControlStub(fd, IOC_GET_FLAGS, 
				0, (Address) NULL, sizeof(value), usp);
	    if (status == SUCCESS) {
		(void)Vm_CopyIn(sizeof(int), usp, (Address)retValPtr);
		*retValPtr = (*retValPtr & IOC_CLOSE_ON_EXEC) ? 1 : 0;
	    }
	    break;

	case F_SETFD:
	    usp -= sizeof(int);
	    value = IOC_CLOSE_ON_EXEC;
	    status = Vm_CopyOut(sizeof(value), (Address)&value, usp);
	    if (status != SUCCESS) {
		break;
	    }
	    if (arg & 1) {
		status = Fs_IOControlStub(fd, IOC_SET_BITS, 
				sizeof(value), usp, 0, (Address) NULL);
	    } else {
		status = Fs_IOControlStub(fd, IOC_CLEAR_BITS, 
				sizeof(value), usp, 0, (Address) NULL);
	    }
	    break;

	case F_GETFL:  {
		int temp;

		usp -= sizeof(int);
		status = Fs_IOControlStub(fd, IOC_GET_FLAGS, 
				0, (Address) NULL, sizeof(temp), usp);
		if (status != SUCCESS) {
		    break;
		}
		(void)Vm_CopyIn(sizeof(temp), usp, (Address)&temp);
		*retValPtr = 0;
		if (temp & IOC_APPEND) {
		    *retValPtr |= FAPPEND;
		}
		if (temp & IOC_NON_BLOCKING) {
		    *retValPtr |= FNDELAY;
		}
		if (temp & IOC_ASYNCHRONOUS) {
		    *retValPtr |= FASYNC;
		}
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
	    if (value == 0) {
		status = SUCCESS;
	    } else {
		usp -= sizeof(int);
		status = Vm_CopyOut(sizeof(value), &value, usp);
		if (status != SUCCESS) {
		    break;
		}
		status = Fs_IOControlStub(fd, IOC_SET_BITS, 
			    sizeof(value), usp, 0, (Address) NULL);
	    }
	    break;

	case F_GETOWN: {
		Ioc_Owner owner;

		usp -= sizeof(owner);
		status = Fs_IOControlStub(fd, IOC_GET_OWNER, 
					  0, (Address) NULL,
					  sizeof(owner), usp);
		if (status != SUCCESS) {
		    break;
		}
		(void)Vm_CopyIn(sizeof(owner), usp, &owner);
		if (owner.procOrFamily == IOC_OWNER_FAMILY) {
		    *retValPtr = -owner.id;
		} else {
		    *retValPtr = owner.id;
		}
	    }
	    break;

	case F_SETOWN: {
		Ioc_Owner owner;

		usp -= sizeof(owner);
		if (arg < 0) {
		    owner.id = -arg;
		    owner.procOrFamily = IOC_OWNER_FAMILY;
		} else {
		    owner.id = arg;
		    owner.procOrFamily = IOC_OWNER_PROC;
		}
		status = Vm_CopyOut(sizeof(owner), (Address)&owner, usp);
		if (status != SUCCESS) {
		    break;
		}
		status = Fs_IOControlStub(fd, IOC_SET_OWNER, 
				sizeof(owner), usp, 0, (Address) NULL);
	    }
	    break;

	default:
	    break;
    }
    return(status);
}
