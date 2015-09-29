/* 
 * getpgrp.c --
 *
 *	Procedure to map from Unix getpgrp system call to Sprite.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: getpgrp.c,v 1.1 88/06/19 14:31:26 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "compatInt.h"
#include "proc.h"


/*
 *----------------------------------------------------------------------
 *
 * getpgrp --
 *
 *	Procedure to map from Unix getpgrp system call to Sprite 
 *	Proc_GetFamilyID. 
 *
 * Results:
 *	UNIX_ERROR is returned upon error, with the actual error code
 *	stored in errno.  Otherwise the family id of the given process is
 *	returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
getpgrp(pid)
    int pid;			/* Process to get the process group for. */
{
    ReturnStatus status;	/* result returned by Proc_GetFamilyID */
    int		 familyID;	/* Family ID of process. */

    if (pid == 0) {
	pid = PROC_MY_PID;
    }
    status = Proc_GetFamilyID(pid, &familyID);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	return(familyID);
    }
}
