/* 
 * setpgrp.c --
 *
 *	Procedure to map from Unix setpgrp system call to Sprite.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: setpgrp.c,v 1.1 88/06/19 14:31:57 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "compatInt.h"
#include "proc.h"


/*
 *----------------------------------------------------------------------
 *
 * setpgrp --
 *
 *	Procedure to map from Unix setpgrp system call to Sprite 
 *	Proc_SetFamilyID. 
 *
 * Results:
 *	UNIX_ERROR is returned upon error, with the actual error code
 *	stored in errno.  Otherwise UNIX_SUCCESS is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
setpgrp(pid, pgrp)
    int pid;			/* Process to set the pid for. */
    int pgrp;			/* Name of group. */
{
    ReturnStatus status;	/* result returned by Proc_SetFamilyID */

    if (pid == 0) {
	pid = PROC_MY_PID;
    }

    status = Proc_SetFamilyID(pid, pgrp);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	return(UNIX_SUCCESS);
    }
}
